#include "style/style.h"

#include "data/tileSource.h"
#include "gl/renderState.h"
#include "gl/shaderProgram.h"
#include "gl/mesh.h"
#include "log.h"
#include "map.h"
#include "marker/marker.h"
#include "scene/light.h"
#include "scene/scene.h"
#include "scene/spriteAtlas.h"
#include "scene/styleParam.h"
#include "style/material.h"
#include "tile/tile.h"
#include "view/view.h"

#include "rasters_glsl.h"

namespace Tangram {

Style::Style(std::string _name, Blending _blendMode, GLenum _drawMode, bool _selection) :
    m_name(_name),
    m_shaderSource(std::make_unique<ShaderSource>()),
    m_blend(_blendMode),
    m_drawMode(_drawMode),
    m_selection(_selection) {}

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

    if (m_blend == Blending::inlay) {
        m_shaderSource->addSourceBlock("defines", "#define TANGRAM_BLEND_INLAY\n", false);
    } else if (m_blend == Blending::overlay) {
        m_shaderSource->addSourceBlock("defines", "#define TANGRAM_BLEND_OVERLAY\n", false);
    }

    if (m_material.material) {
        m_material.uniforms = m_material.material->injectOnProgram(*m_shaderSource);
    }

    if (m_lightingType != LightingType::none) {

        switch (m_lightingType) {
        case LightingType::vertex:
            m_shaderSource->addSourceBlock("defines", "#define TANGRAM_LIGHTING_VERTEX\n", false);
            break;
        case LightingType::fragment:
            m_shaderSource->addSourceBlock("defines", "#define TANGRAM_LIGHTING_FRAGMENT\n", false);
            break;
        default:
            break;
        }

        for (auto& light : _scene.lights()) {
            auto uniforms = light->getUniforms();
            if (uniforms) {
                m_lights.emplace_back(light.get(), std::move(uniforms));
            }
        }
        for (auto& block : _scene.lightBlocks()) {
            m_shaderSource->addSourceBlock(block.first, block.second);
        }
    }

    if (m_rasterType != RasterType::none) {
        int numRasterSource = 0;
        for (const auto& source : _scene.tileSources()) {
            if (source->isRaster()) { numRasterSource++; }
        }
        if (numRasterSource > 0) {
            // Inject shader defines for raster sampling and uniforms
            if (m_rasterType == RasterType::normal) {
                m_shaderSource->addSourceBlock("defines", "#define TANGRAM_RASTER_TEXTURE_NORMAL\n", false);
            } else if (m_rasterType == RasterType::color) {
                m_shaderSource->addSourceBlock("defines", "#define TANGRAM_RASTER_TEXTURE_COLOR\n", false);
            }

            m_shaderSource->addSourceBlock("defines", "#define TANGRAM_NUM_RASTER_SOURCES "
                                           + std::to_string(numRasterSource) + "\n", false);
            m_shaderSource->addSourceBlock("defines", "#define TANGRAM_MODEL_POSITION_BASE_ZOOM_VARYING\n", false);

            m_shaderSource->addSourceBlock("raster", rasters_glsl);
        }
    }

    const auto& blocks = m_shaderSource->getSourceBlocks();
    if (blocks.find("color") != blocks.end() ||
        blocks.find("filter") != blocks.end() ||
        blocks.find("raster") != blocks.end()) {
        m_hasColorShaderBlock = true;
    }

    std::string vertSrc = m_shaderSource->buildVertexSource();
    std::string fragSrc = m_shaderSource->buildFragmentSource();

    for (auto& s : _scene.styles()) {
        auto& prg = s->m_shaderProgram;
        if (!prg) { break; }
        if (prg->vertexShaderSource() == vertSrc &&
            prg->fragmentShaderSource() == fragSrc) {
            m_shaderProgram = prg;
            break;
        }
    }
    if (!m_shaderProgram) {
        m_shaderProgram = std::make_shared<ShaderProgram>();
        m_shaderProgram->setDescription("{style:" + m_name + "}");
        m_shaderProgram->setShaderSource(vertSrc, fragSrc);
    }

    if (m_selection) {
        std::string vertSrc = m_shaderSource->buildSelectionVertexSource();
        std::string fragSrc = m_shaderSource->buildSelectionFragmentSource();

        for (auto& s : _scene.styles()) {
            if (!s->m_selection) { continue; }

            auto& prg = s->m_selectionProgram;
            if (!prg) { break; }
            if (prg->vertexShaderSource() == vertSrc &&
                prg->fragmentShaderSource() == fragSrc) {
                m_selectionProgram = prg;
                break;
            }
        }
        if (!m_selectionProgram) {
            m_selectionProgram = std::make_shared<ShaderProgram>();
            m_selectionProgram->setDescription("selection_program {style:" + m_name + "}");
            m_selectionProgram->setShaderSource(vertSrc, fragSrc);
        }
    }

    // Clear ShaderSource builder
    m_shaderSource.reset();
}

void Style::setLightingType(LightingType _type) {
    m_lightingType = _type;
}

void Style::setupSceneShaderUniforms(RenderState& rs, Scene& _scene, UniformBlock& _uniformBlock) {
    for (auto& uniformPair : _uniformBlock.styleUniforms) {
        const auto& name = uniformPair.first;
        auto& value = uniformPair.second;

        if (value.is<UniformTexture>()) {
            auto& texture  = value.get<UniformTexture>();
            if (!texture) { continue; }

            texture->bind(rs, rs.nextAvailableTextureUnit());

            m_shaderProgram->setUniformi(rs, name, rs.currentTextureUnit());
        } else if (value.is<bool>()) {
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

            for (const auto& texture : textureUniformArray.textures) {
                if (!texture) { continue; }

                texture->bind(rs, rs.nextAvailableTextureUnit());

                textureUniformArray.slots.push_back(rs.currentTextureUnit());
            }

            m_shaderProgram->setUniformi(rs, name, textureUniformArray);
        }
    }
}

void Style::setupShaderUniforms(RenderState& rs, ShaderProgram& _program, const View& _view,
                                Scene& _scene, UniformBlock& _uniforms) {

    // Reset the currently used texture unit to 0
    rs.resetTextureUnit();

    // Set time uniforms style's shader programs
    _program.setUniformf(rs, _uniforms.uTime, _scene.time());

    _program.setUniformf(rs, _uniforms.uDevicePixelRatio, m_pixelScale);

    if (m_material.uniforms) {
        m_material.material->setupProgram(rs, *m_shaderProgram, *m_material.uniforms);
    }

    // Set up lights
    for (const auto& light : m_lights) {
        light.light->setupProgram(rs, _view, *m_shaderProgram, *light.uniforms);
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
            rs.depthMask(GL_FALSE);
            break;
        case Blending::multiply:
            rs.blending(GL_TRUE);
            rs.blendingFunc(GL_ZERO, GL_SRC_COLOR);
            rs.depthTest(GL_TRUE);
            rs.depthMask(GL_FALSE);
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
        case Blending::translucent:
            rs.blending(GL_TRUE);
            rs.blendingFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            rs.depthTest(GL_TRUE);
            rs.depthMask(GL_TRUE);
            break;
        default:
            break;
    }
}

void Style::drawSelectionFrame(RenderState& rs, const View& _view, Scene& _scene,
                               const std::vector<std::shared_ptr<Tile>>& _tiles,
                               const std::vector<std::unique_ptr<Marker>>& _markers) {

    onBeginDrawSelectionFrame(rs, _view, _scene);

    for (const auto& tile : _tiles) { drawSelectionFrame(rs, *tile); }
    for (const auto& marker : _markers) { drawSelectionFrame(rs, *marker); }
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
        case Blending::translucent:
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

void Style::drawSelectionFrame(RenderState& _rs, const Marker& _marker) {
    if (!m_selection || _marker.styleId() != m_id || !_marker.isVisible()) {
        return;
    }

    auto* mesh = _marker.mesh();

    if (!mesh) { return; }

    m_selectionProgram->setUniformMatrix4f(_rs, m_selectionUniforms.uModel, _marker.modelMatrix());
    m_selectionProgram->setUniformf(_rs, m_selectionUniforms.uTileOrigin,
                                    _marker.origin().x, _marker.origin().y,
                                    _marker.builtZoomLevel(), _marker.builtZoomLevel());

    if (!mesh->draw(_rs, *m_selectionProgram, false)) {
        LOGN("Mesh built by style %s cannot be drawn", m_name.c_str());
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

bool Style::draw(RenderState& rs, const View& _view, Scene& _scene,
                 const std::vector<std::shared_ptr<Tile>>& _tiles,
                 const std::vector<std::unique_ptr<Marker>>& _markers) {

    auto tileIt = std::find_if(std::begin(_tiles), std::end(_tiles),
                               [this](const auto& t){ return bool(t->getMesh(*this)); });

    auto markerIt = std::find_if(std::begin(_markers), std::end(_markers),
                               [this](const auto& m){ return m->styleId() == this->m_id && m->mesh(); });

    bool meshDrawn = false;

    // Skip when no mesh is to be rendered.
    // This also compiles shaders when they are first used.
    if (tileIt == std::end(_tiles) && markerIt == std::end(_markers)) {
        return false;
    }

    onBeginDrawFrame(rs, _view, _scene);

    if (m_blend == Blending::translucent) {
        rs.colorMask(false, false, false, false);
    }

    for (const auto& tile : _tiles) {
        meshDrawn |= draw(rs, *tile);
    }
    for (const auto& marker : _markers) {
        meshDrawn |= draw(rs, *marker);
    }

    if (meshDrawn) {
        if (m_blend == Blending::translucent) {
            rs.colorMask(true, true, true, true);
            GL::depthFunc(GL_EQUAL);

            GL::enable(GL_STENCIL_TEST);
            GL::clear(GL_STENCIL_BUFFER_BIT);
            GL::stencilFunc(GL_EQUAL, GL_ZERO, 0xFF);
            GL::stencilOp(GL_KEEP, GL_KEEP, GL_INCR);

            for (const auto &tile : _tiles) { draw(rs, *tile); }
            for (const auto &marker : _markers) { draw(rs, *marker); }

            GL::disable(GL_STENCIL_TEST);
            GL::depthFunc(GL_LESS);
        }
    }

    onEndDrawFrame(rs, _view, _scene);

    return meshDrawn;
}


bool Style::draw(RenderState& rs, const Tile& _tile) {

    auto& styleMesh = _tile.getMesh(*this);

    if (!styleMesh) { return false; }

    bool styleMeshDrawn = true;
    TileID tileID = _tile.getID();

    if (hasRasters()) {
        UniformTextureArray textureIndexUniform;
        UniformArray2f rasterSizeUniform;
        UniformArray3f rasterOffsetsUniform;

        for (auto& raster : _tile.rasters()) {

            auto& texture = raster.texture;
            auto texUnit = rs.nextAvailableTextureUnit();
            texture->bind(rs, texUnit);

            textureIndexUniform.slots.push_back(texUnit);
            rasterSizeUniform.push_back({texture->width(), texture->height()});

            float x = 0.f;
            float y = 0.f;
            float z = 1.f;

            if (tileID.z > raster.tileID.z) {
                float dz = tileID.z - raster.tileID.z;
                float dz2 = powf(2.f, dz);
                x = fmodf(tileID.x, dz2) / dz2;
                y = (dz2 - 1.f - fmodf(tileID.y, dz2)) / dz2;
                z = 1.f / dz2;
            }
            rasterOffsetsUniform.emplace_back(x, y, z);
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
        styleMeshDrawn = false;
    }

    if (hasRasters()) {
        for (auto& raster : _tile.rasters()) {
            if (raster.isValid()) {
                rs.releaseTextureUnit();
            }
        }
    }

    return styleMeshDrawn;
}

bool Style::draw(RenderState& rs, const Marker& marker) {

    if (marker.styleId() != m_id || !marker.isVisible()) { return false; }

    auto* mesh = marker.mesh();

    if (!mesh) { return false; }
    bool styleMeshDrawn = true;

    m_shaderProgram->setUniformMatrix4f(rs, m_mainUniforms.uModel, marker.modelMatrix());
    m_shaderProgram->setUniformf(rs, m_mainUniforms.uTileOrigin,
                                 marker.origin().x, marker.origin().y,
                                 marker.builtZoomLevel(), marker.builtZoomLevel());

    if (!mesh->draw(rs, *m_shaderProgram)) {
        LOGN("Mesh built by style %s cannot be drawn", m_name.c_str());
        styleMeshDrawn = false;
    }

    return styleMeshDrawn;
}

void Style::setDefaultDrawRule(std::unique_ptr<DrawRuleData>&& _rule) {
    m_defaultDrawRule = std::move(_rule);
}

void Style::applyDefaultDrawRules(DrawRule& _rule) const {
    if (m_defaultDrawRule) {
        for (auto& param : m_defaultDrawRule->parameters) {
            auto key = static_cast<uint8_t>(param.key);
            if (!_rule.active[key]) {
                _rule.active[key] = true;
                // NOTE: layername and layer depth are actually immaterial here, since these are
                // only used during layer draw rules merging. Adding a default string for
                // debugging purposes.
                _rule.params[key] = { &param, "default_style_draw_rule", 0 };
            }
        }
    }
}

bool StyleBuilder::checkRule(const DrawRule& _rule) const {

    uint32_t checkColor;
    uint32_t checkOrder;

    if (!_rule.get(StyleParamKey::color, checkColor)) {
        if (!style().hasColorShaderBlock()) {
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
