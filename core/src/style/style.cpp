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
#include "data/dataSource.h"
#include "view/view.h"
#include "tangram.h"

#include "shaders/rasters_glsl.h"

namespace Tangram {

Style::Style(std::string _name, Blending _blendMode, GLenum _drawMode) :
    m_name(_name),
    m_shaderProgram(std::make_unique<ShaderProgram>()),
    m_blend(_blendMode),
    m_drawMode(_drawMode) {
    m_material.material = std::make_shared<Material>();
}

Style::~Style() {}

Style::LightHandle::LightHandle(Light* _light, std::unique_ptr<LightUniforms> _uniforms)
    : light(_light), uniforms(std::move(_uniforms)){}

const std::vector<std::string>& Style::builtInStyleNames() {
    static std::vector<std::string> builtInStyleNames{ "points", "lines", "polygons", "text", "debug", "debugtext" };
    return builtInStyleNames;
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

    setupRasters(_scene.dataSources());
}

void Style::setMaterial(const std::shared_ptr<Material>& _material) {
    m_material.material = _material;
    m_material.uniforms.reset();
}

void Style::setLightingType(LightingType _type) {
    m_lightingType = _type;
}

void Style::setupShaderUniforms(Scene& _scene) {
    for (auto& uniformPair : m_styleUniforms) {
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

            texture->update(RenderState::nextAvailableTextureUnit());
            texture->bind(RenderState::currentTextureUnit());

            m_shaderProgram->setUniformi(name, RenderState::currentTextureUnit());
        } else {

            if (value.is<bool>()) {
                m_shaderProgram->setUniformi(name, value.get<bool>());
            } else if(value.is<float>()) {
                m_shaderProgram->setUniformf(name, value.get<float>());
            } else if(value.is<glm::vec2>()) {
                m_shaderProgram->setUniformf(name, value.get<glm::vec2>());
            } else if(value.is<glm::vec3>()) {
                m_shaderProgram->setUniformf(name, value.get<glm::vec3>());
            } else if(value.is<glm::vec4>()) {
                m_shaderProgram->setUniformf(name, value.get<glm::vec4>());
            } else if (value.is<UniformArray1f>()) {
                m_shaderProgram->setUniformf(name, value.get<UniformArray1f>());
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

                    texture->update(RenderState::nextAvailableTextureUnit());
                    texture->bind(RenderState::currentTextureUnit());

                    textureUniformArray.slots.push_back(RenderState::currentTextureUnit());
                }

                m_shaderProgram->setUniformi(name, textureUniformArray);
            }
        }
    }
}

void Style::setupRasters(const std::vector<std::shared_ptr<DataSource>>& _dataSources) {
    if (!hasRasters()) {
        return;
    }

    int numRasterSource = 0;
    for (const auto& dataSource : _dataSources) {
        if (dataSource->isRaster()) {
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

void Style::onBeginDrawFrame(const View& _view, Scene& _scene) {

    // Reset the currently used texture unit to 0
    RenderState::resetTextureUnit();

    // Set time uniforms style's shader programs
    m_shaderProgram->setUniformf(m_uTime, _scene.time());

    m_shaderProgram->setUniformf(m_uDevicePixelRatio, m_pixelScale);

    if (m_material.uniforms) {
        m_material.material->setupProgram(*m_material.uniforms);
    }

    // Set up lights
    for (const auto& light : m_lights) {
        light.light->setupProgram(_view, *light.uniforms);
    }

    // Set Map Position
    m_shaderProgram->setUniformf(m_uResolution, _view.getWidth(), _view.getHeight());

    const auto& mapPos = _view.getPosition();
    m_shaderProgram->setUniformf(m_uMapPosition, mapPos.x, mapPos.y, _view.getZoom());
    m_shaderProgram->setUniformMatrix3f(m_uNormalMatrix, _view.getNormalMatrix());
    m_shaderProgram->setUniformMatrix3f(m_uInverseNormalMatrix, _view.getInverseNormalMatrix());
    m_shaderProgram->setUniformf(m_uMetersPerPixel, 1.0 / _view.pixelsPerMeter());
    m_shaderProgram->setUniformMatrix4f(m_uView, _view.getViewMatrix());
    m_shaderProgram->setUniformMatrix4f(m_uProj, _view.getProjectionMatrix());

    setupShaderUniforms(_scene);

    // Configure render state
    switch (m_blend) {
        case Blending::none:
            RenderState::blending(GL_FALSE);
            RenderState::blendingFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            RenderState::depthTest(GL_TRUE);
            RenderState::depthWrite(GL_TRUE);
            break;
        case Blending::add:
            RenderState::blending(GL_TRUE);
            RenderState::blendingFunc(GL_ONE, GL_ONE);
            RenderState::depthTest(GL_FALSE);
            RenderState::depthWrite(GL_TRUE);
            break;
        case Blending::multiply:
            RenderState::blending(GL_TRUE);
            RenderState::blendingFunc(GL_ZERO, GL_SRC_COLOR);
            RenderState::depthTest(GL_FALSE);
            RenderState::depthWrite(GL_TRUE);
            break;
        case Blending::overlay:
            RenderState::blending(GL_TRUE);
            RenderState::blendingFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            RenderState::depthTest(GL_FALSE);
            RenderState::depthWrite(GL_FALSE);
            break;
        case Blending::inlay:
            // TODO: inlay does not behave correctly for labels because they don't have a z position
            RenderState::blending(GL_TRUE);
            RenderState::blendingFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            RenderState::depthTest(GL_TRUE);
            RenderState::depthWrite(GL_FALSE);
            break;
        default:
            break;
    }
}

void Style::draw(const Tile& _tile) {

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
                auto texUnit = RenderState::nextAvailableTextureUnit();
                texture->update(texUnit);
                texture->bind(texUnit);

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

        m_shaderProgram->setUniformi(m_uRasters, textureIndexUniform);
        m_shaderProgram->setUniformf(m_uRasterSizes, rasterSizeUniform);
        m_shaderProgram->setUniformf(m_uRasterOffsets, rasterOffsetsUniform);
    }

    m_shaderProgram->setUniformMatrix4f(m_uModel, _tile.getModelMatrix());
    m_shaderProgram->setUniformf(m_uProxyDepth, _tile.isProxy() ? 1.f : 0.f);
    m_shaderProgram->setUniformf(m_uTileOrigin,
                                 _tile.getOrigin().x,
                                 _tile.getOrigin().y,
                                 tileID.s,
                                 tileID.z);

    if (!styleMesh->draw(*m_shaderProgram)) {
        LOGN("Mesh built by style %s cannot be drawn", m_name.c_str());
    }

    if (hasRasters()) {
        for (auto& raster : _tile.rasters()) {
            if (raster.isValid()) {
                RenderState::releaseTextureUnit();
            }
        }
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

void StyleBuilder::addFeature(const Feature& _feat, const DrawRule& _rule) {

    if (!checkRule(_rule)) { return; }

    switch (_feat.geometryType) {
        case GeometryType::points:
            for (auto& point : _feat.points) {
                addPoint(point, _feat.props, _rule);
            }
            break;
        case GeometryType::lines:
            for (auto& line : _feat.lines) {
                addLine(line, _feat.props, _rule);
            }
            break;
        case GeometryType::polygons:
            for (auto& polygon : _feat.polygons) {
                addPolygon(polygon, _feat.props, _rule);
            }
            break;
        default:
            break;
    }

}

StyleBuilder::StyleBuilder(const Style& _style) {
    const auto& blocks = _style.getShaderProgram()->getSourceBlocks();
    if (blocks.find("color") != blocks.end() ||
        blocks.find("filter") != blocks.end() ||
        blocks.find("raster") != blocks.end()) {
        m_hasColorShaderBlock = true;
    }
}

void StyleBuilder::addPoint(const Point& _point, const Properties& _props, const DrawRule& _rule) {
    // No-op by default
}

void StyleBuilder::addLine(const Line& _line, const Properties& _props, const DrawRule& _rule) {
    // No-op by default
}

void StyleBuilder::addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) {
    // No-op by default
}

}
