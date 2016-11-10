#include "style.h"

#include "material.h"
#include "gl/renderState.h"
#include "gl/shaderProgram.h"
#include "gl/mesh.h"
#include "scene/light.h"
#include "scene/styleParam.h"
#include "scene/drawRule.h"
#include "scene/scene.h"
#include "scene/spriteAtlas.h"
#include "tile/tile.h"
#include "data/tileSource.h"
#include "view/view.h"
#include "marker/marker.h"
#include "tangram.h"
#include "log.h"

#include "shaders/rasters_glsl.h"
#include "shaders/selection_fs.h"

namespace Tangram {

Style::Style(std::string _name, Blending _blendMode, GLenum _drawMode, bool _selection) :
    m_name(_name),
    m_shaderProgram(std::make_unique<ShaderProgram>()),
    m_selectionProgram(std::make_unique<ShaderProgram>()),
    m_blend(_blendMode),
    m_drawMode(_drawMode),
    m_selection(_selection) {
    m_material.material = std::make_shared<Material>();
}

Style::~Style() {}

Style::LightHandle::LightHandle(Light* _light, std::unique_ptr<LightUniforms> _uniforms)
    : light(_light), uniforms(std::move(_uniforms)){}

const std::vector<std::string>& Style::builtInStyleNames() {
    static std::vector<std::string> builtInStyleNames{ "points", "lines", "polygons", "text", "debug", "debugtext" };
    return builtInStyleNames;
}

void Style::constructSelectionShaderProgram() {

    if (m_selection) {
        m_selectionProgram->setDescription("selection_program {style:" + m_name + "}");
        m_selectionProgram->setSourceStrings(SHADER_SOURCE(selection_fs),
                                             m_shaderProgram->getVertexShaderSource());

        m_selectionProgram->addSourceBlock("defines", "#define TANGRAM_FEATURE_SELECTION\n", false);
    }
}

void Style::build(const Scene& _scene) {

    constructVertexLayout();
    constructShaderProgram();

    m_shaderProgram->setDescription("{style:" + m_name + "}");

    switch (m_lightingType) {
        case LightingType::vertex:
            m_shaderProgram->addSourceBlock("defines", "#define TANGRAM_LIGHTING_VERTEX\n", false);
            break;
        case LightingType::fragment:
            m_shaderProgram->addSourceBlock("defines", "#define TANGRAM_LIGHTING_FRAGMENT\n", false);
            break;
        default:
            break;
    }

    if (m_blend == Blending::inlay) {
        m_shaderProgram->addSourceBlock("defines", "#define TANGRAM_BLEND_INLAY\n", false);
    } else if (m_blend == Blending::overlay) {
        m_shaderProgram->addSourceBlock("defines", "#define TANGRAM_BLEND_OVERLAY\n", false);
    }

    if (m_material.material) {
        m_material.uniforms = m_material.material->injectOnProgram(*m_shaderProgram);
    }

    if (m_lightingType != LightingType::none) {
        for (auto& light : _scene.lights()) {
            auto uniforms = light->injectOnProgram(*m_shaderProgram);
            if (uniforms) {
                m_lights.emplace_back(light.get(), std::move(uniforms));
            }
        }
    }

    setupRasters(_scene.tileSources());

    constructSelectionShaderProgram();
}

void Style::setMaterial(const std::shared_ptr<Material>& _material) {
    m_material.material = _material;
    m_material.uniforms.reset();
}

void Style::setLightingType(LightingType _type) {
    m_lightingType = _type;
}

void Style::setupSceneShaderUniforms(RenderState& rs, Scene& _scene, UniformBlock& _uniformBlock) {
    for (auto& uniformPair : _uniformBlock.styleUniforms) {
        const auto& name = uniformPair.first;
        auto& value = uniformPair.second;

        if (value.is<std::string>()) {
            std::string textureName = value.get<std::string>();
            std::shared_ptr<Texture> texture = _scene.getTexture(textureName);

            if (!texture) {
                LOGN("Texture with texture name %s is not available to be sent as uniform",
                    textureName.c_str());
                continue;
            }

            texture->update(rs, rs.nextAvailableTextureUnit());
            texture->bind(rs, rs.currentTextureUnit());

            m_shaderProgram->setUniformi(rs, name, rs.currentTextureUnit());
        } else {

            if (value.is<bool>()) {
                m_shaderProgram->setUniformi(rs, name, value.get<bool>());
            } else if(value.is<float>()) {
                m_shaderProgram->setUniformf(rs, name, value.get<float>());
            } else if(value.is<glm::vec2>()) {
                m_shaderProgram->setUniformf(rs, name, value.get<glm::vec2>());
            } else if(value.is<glm::vec3>()) {
                m_shaderProgram->setUniformf(rs, name, value.get<glm::vec3>());
            } else if(value.is<glm::vec4>()) {
                m_shaderProgram->setUniformf(rs, name, value.get<glm::vec4>());
            } else if (value.is<UniformArray1f>()) {
                m_shaderProgram->setUniformf(rs, name, value.get<UniformArray1f>());
            } else if (value.is<UniformTextureArray>()) {
                UniformTextureArray& textureUniformArray = value.get<UniformTextureArray>();
                textureUniformArray.slots.clear();

                for (const auto& textureName : textureUniformArray.names) {
                    std::shared_ptr<Texture> texture = _scene.getTexture(textureName);

                    if (!texture) {
                        LOGN("Texture with texture name %s is not available to be sent as uniform",
                            textureName.c_str());
                        continue;
                    }

                    texture->update(rs, rs.nextAvailableTextureUnit());
                    texture->bind(rs, rs.currentTextureUnit());

                    textureUniformArray.slots.push_back(rs.currentTextureUnit());
                }

                m_shaderProgram->setUniformi(rs, name, textureUniformArray);
            }
        }
    }
}

void Style::setupRasters(const std::vector<std::shared_ptr<TileSource>>& _sources) {
    if (!hasRasters()) {
        return;
    }

    int numRasterSource = 0;
    for (const auto& source : _sources) {
        if (source->isRaster()) {
            numRasterSource++;
        }
    }

    if (numRasterSource == 0) {
        return;
    }

    // Inject shader defines for raster sampling and uniforms
    if (m_rasterType == RasterType::normal) {
        m_shaderProgram->addSourceBlock("defines", "#define TANGRAM_RASTER_TEXTURE_NORMAL\n", false);
    } else if (m_rasterType == RasterType::color) {
        m_shaderProgram->addSourceBlock("defines", "#define TANGRAM_RASTER_TEXTURE_COLOR\n", false);
    }

    m_shaderProgram->addSourceBlock("defines", "#define TANGRAM_NUM_RASTER_SOURCES "
            + std::to_string(numRasterSource) + "\n", false);
    m_shaderProgram->addSourceBlock("defines", "#define TANGRAM_MODEL_POSITION_BASE_ZOOM_VARYING\n", false);

    m_shaderProgram->addSourceBlock("raster", SHADER_SOURCE(rasters_glsl));
}


void Style::setupShaderUniforms(RenderState& rs, ShaderProgram& _program, const View& _view, Scene& _scene, UniformBlock& _uniforms) {

    // Reset the currently used texture unit to 0
    rs.resetTextureUnit();

    // Set time uniforms style's shader programs
    _program.setUniformf(rs, _uniforms.uTime, _scene.time());

    _program.setUniformf(rs, _uniforms.uDevicePixelRatio, m_pixelScale);

    if (m_material.uniforms) {
        m_material.material->setupProgram(rs, *m_material.uniforms);
    }

    // Set up lights
    for (const auto& light : m_lights) {
        light.light->setupProgram(rs, _view, *light.uniforms);
    }

    // Set Map Position
    _program.setUniformf(rs, _uniforms.uResolution, _view.getWidth(), _view.getHeight());

    const auto& mapPos = _view.getPosition();
    _program.setUniformf(rs, _uniforms.uMapPosition, mapPos.x, mapPos.y, _view.getZoom());
    _program.setUniformMatrix3f(rs, _uniforms.uNormalMatrix, _view.getNormalMatrix());
    _program.setUniformMatrix3f(rs, _uniforms.uInverseNormalMatrix, _view.getInverseNormalMatrix());
    _program.setUniformf(rs, _uniforms.uMetersPerPixel, 1.0 / _view.pixelsPerMeter());
    _program.setUniformMatrix4f(rs, _uniforms.uView, _view.getViewMatrix());
    _program.setUniformMatrix4f(rs, _uniforms.uProj, _view.getProjectionMatrix());

    setupSceneShaderUniforms(rs, _scene, _uniforms);

}

void Style::onBeginDrawFrame(RenderState& rs, const View& _view, Scene& _scene) {

    setupShaderUniforms(rs, *m_shaderProgram, _view, _scene, m_mainUniforms);

    // Configure render state
    switch (m_blend) {
        case Blending::opaque:
            rs.blending(GL_FALSE);
            rs.blendingFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            rs.depthTest(GL_TRUE);
            rs.depthMask(GL_TRUE);
            break;
        case Blending::add:
            rs.blending(GL_TRUE);
            rs.blendingFunc(GL_ONE, GL_ONE);
            rs.depthTest(GL_TRUE);
            rs.depthMask(GL_TRUE);
            break;
        case Blending::multiply:
            rs.blending(GL_TRUE);
            rs.blendingFunc(GL_ZERO, GL_SRC_COLOR);
            rs.depthTest(GL_TRUE);
            rs.depthMask(GL_TRUE);
            break;
        case Blending::overlay:
            rs.blending(GL_TRUE);
            rs.blendingFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            rs.depthTest(GL_FALSE);
            rs.depthMask(GL_FALSE);
            break;
        case Blending::inlay:
            // TODO: inlay does not behave correctly for labels because they don't have a z position
            rs.blending(GL_TRUE);
            rs.blendingFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            rs.depthTest(GL_TRUE);
            rs.depthMask(GL_FALSE);
            break;
        default:
            break;
    }
}

void Style::onBeginDrawSelectionFrame(RenderState& rs, const View& _view, Scene& _scene) {

    if (!m_selection) {
        return;
    }

    setupShaderUniforms(rs, *m_selectionProgram, _view, _scene, m_selectionUniforms);

    // Configure render state
    rs.blending(GL_FALSE);

    switch (m_blend) {
        case Blending::opaque:
        case Blending::add:
        case Blending::multiply:
            rs.depthTest(GL_TRUE);
            rs.depthMask(GL_TRUE);
            break;
        case Blending::overlay:
            rs.depthTest(GL_FALSE);
            rs.depthMask(GL_FALSE);
            break;
        case Blending::inlay:
            rs.depthTest(GL_TRUE);
            rs.depthMask(GL_FALSE);
            break;
        default:
            break;
    }
}

void Style::drawSelectionFrame(Tangram::RenderState& rs, const Tangram::Tile &_tile) {

    if (!m_selection) {
        return;
    }

    auto& styleMesh = _tile.getMesh(*this);

    if (!styleMesh) { return; }

    TileID tileID = _tile.getID();

    m_selectionProgram->setUniformMatrix4f(rs, m_selectionUniforms.uModel, _tile.getModelMatrix());
    m_selectionProgram->setUniformf(rs, m_selectionUniforms.uProxyDepth, _tile.isProxy() ? 1.f : 0.f);
    m_selectionProgram->setUniformf(rs, m_selectionUniforms.uTileOrigin,
                                    _tile.getOrigin().x,
                                    _tile.getOrigin().y,
                                    tileID.s,
                                    tileID.z);

    if (!styleMesh->draw(rs, *m_selectionProgram, false)) {
        LOGN("Mesh built by style %s cannot be drawn", m_name.c_str());
    }

}

void Style::draw(RenderState& rs, const Tile& _tile) {

    auto& styleMesh = _tile.getMesh(*this);

    if (!styleMesh) { return; }

    TileID tileID = _tile.getID();

    if (hasRasters() && !_tile.rasters().empty()) {
        UniformTextureArray textureIndexUniform;
        UniformArray2f rasterSizeUniform;
        UniformArray3f rasterOffsetsUniform;

        for (auto& raster : _tile.rasters()) {
            if (raster.isValid()) {
                auto& texture = raster.texture;
                auto texUnit = rs.nextAvailableTextureUnit();
                texture->update(rs, texUnit);
                texture->bind(rs, texUnit);

                textureIndexUniform.slots.push_back(texUnit);
                rasterSizeUniform.push_back({texture->getWidth(), texture->getHeight()});

                if (tileID.z > raster.tileID.z) {
                    float dz = tileID.z - raster.tileID.z;
                    float dz2 = powf(2.f, dz);

                    rasterOffsetsUniform.push_back({
                            fmodf(tileID.x, dz2) / dz2,
                                (dz2 - 1.f - fmodf(tileID.y, dz2)) / dz2,
                                1.f / dz2
                                });
                } else {
                    rasterOffsetsUniform.push_back({0, 0, 1});
                }
            }
        }

        m_shaderProgram->setUniformi(rs, m_mainUniforms.uRasters, textureIndexUniform);
        m_shaderProgram->setUniformf(rs, m_mainUniforms.uRasterSizes, rasterSizeUniform);
        m_shaderProgram->setUniformf(rs, m_mainUniforms.uRasterOffsets, rasterOffsetsUniform);
    }

    m_shaderProgram->setUniformMatrix4f(rs, m_mainUniforms.uModel, _tile.getModelMatrix());
    m_shaderProgram->setUniformf(rs, m_mainUniforms.uProxyDepth, _tile.isProxy() ? 1.f : 0.f);
    m_shaderProgram->setUniformf(rs, m_mainUniforms.uTileOrigin,
                                 _tile.getOrigin().x,
                                 _tile.getOrigin().y,
                                 tileID.s,
                                 tileID.z);

    if (!styleMesh->draw(rs, *m_shaderProgram)) {
        LOGN("Mesh built by style %s cannot be drawn", m_name.c_str());
    }

    if (hasRasters()) {
        for (auto& raster : _tile.rasters()) {
            if (raster.isValid()) {
                rs.releaseTextureUnit();
            }
        }
    }
}

void Style::draw(RenderState& rs, const Marker& marker) {

    if (marker.styleId() != m_id) { return; }

    auto* mesh = marker.mesh();

    if (!mesh) { return; }

    if (!marker.isVisible()) { return; }

    m_shaderProgram->setUniformMatrix4f(rs, m_mainUniforms.uModel, marker.modelMatrix());
    m_shaderProgram->setUniformf(rs, m_mainUniforms.uTileOrigin,
                                 marker.origin().x, marker.origin().y,
                                 marker.builtZoomLevel(), marker.builtZoomLevel());

    if (!mesh->draw(rs, *m_shaderProgram)) {
        LOGN("Mesh built by style %s cannot be drawn", m_name.c_str());
    }
}

bool StyleBuilder::checkRule(const DrawRule& _rule) const {

    uint32_t checkColor;
    uint32_t checkOrder;

    if (!_rule.get(StyleParamKey::color, checkColor)) {
        if (!m_hasColorShaderBlock) {
            return false;
        }
    }

    if (!_rule.get(StyleParamKey::order, checkOrder)) {
        return false;
    }

    return true;
}

bool StyleBuilder::addFeature(const Feature& _feat, const DrawRule& _rule) {

    if (!checkRule(_rule)) { return false; }

    bool added = false;
    switch (_feat.geometryType) {
        case GeometryType::points:
            for (auto& point : _feat.points) {
                added |= addPoint(point, _feat.props, _rule);
            }
            break;
        case GeometryType::lines:
            for (auto& line : _feat.lines) {
                added |= addLine(line, _feat.props, _rule);
            }
            break;
        case GeometryType::polygons:
            for (auto& polygon : _feat.polygons) {
                added |= addPolygon(polygon, _feat.props, _rule);
            }
            break;
        default:
            break;
    }

    return added;
}

StyleBuilder::StyleBuilder(const Style& _style) {
    const auto& blocks = _style.getShaderProgram()->getSourceBlocks();
    if (blocks.find("color") != blocks.end() ||
        blocks.find("filter") != blocks.end() ||
        blocks.find("raster") != blocks.end()) {
        m_hasColorShaderBlock = true;
    }
}

bool StyleBuilder::addPoint(const Point& _point, const Properties& _props, const DrawRule& _rule) {
    // No-op by default
    return false;
}

bool StyleBuilder::addLine(const Line& _line, const Properties& _props, const DrawRule& _rule) {
    // No-op by default
    return false;
}

bool StyleBuilder::addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) {
    // No-op by default
    return false;
}

}
