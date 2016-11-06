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
#include "marker/marker.h"
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

void Style::addSourceBlock(const std::string& _tagName, const std::string& _glslSource, bool _allowDuplicate){

    if (!_allowDuplicate) {
        for (auto& source : m_sourceBlocks[_tagName]) {
            if (_glslSource == source) {
                return;
            }
        }
    }

    size_t start = 0;
    std::string sourceBlock = _glslSource;
    // Certain graphics drivers have issues with shaders having line continuation backslashes "\".
    // Example raster.glsl was having issues on s6 and note2 because of the "\"s in the glsl file.
    // This also makes sure if any "\"s are present in the shaders coming from style sheet will be
    // taken care of.

    // Replace blackslash+newline with spaces (simplification of regex "\\\\\\s*\\n")
    while ((start = sourceBlock.find("\\\n", start)) != std::string::npos) {
        sourceBlock.replace(start, 2, "  ");
        start += 2;
    }
    m_sourceBlocks[_tagName].push_back(sourceBlock);
}

void Style::insertShaderBlock(const std::string& _name, ShaderSource& _out) {
    auto block = m_sourceBlocks.find(_name);
    if (block != m_sourceBlocks.end()) {
        _out << "// ---- " + _name;
        for (auto& s : block->second) { _out << s; }
        _out << "// ----";
    }
}

void Style::initShaderSource(ShaderSource& out, bool _fragment) {
    out.clear();

    insertShaderBlock("extensions", out);
    if (_fragment) {
        out << "#define TANGRAM_FRAGMENT_SHADER";
    } else {
        out << "#define TANGRAM_VERTEX_SHADER";
    }
    out << "#define TANGRAM_EPSILON 0.00001";
    out << "#ifdef GL_ES";
    out << "    precision highp float;";
    out << "    #define LOWP lowp";
    out << "#else";
    out << "    #define LOWP";
    out << "#endif";
    insertShaderBlock("defines", out);
}

void Style::constructShaderProgram() {

    m_shaderProgram->setDescription("{style:" + m_name + "}");

    ShaderSource out;
    initShaderSource(out, false);
    buildVertexShaderSource(out, false);
    auto vertexSource = out.string;

    initShaderSource(out, true);
    buildFragmentShaderSource(out);
    auto fragmentSource = out.string;

    // LOG(">>> VERTEX %s\n%s\n<<<<<<", m_name.c_str(), vertexSource.c_str());
    // LOG(">>> FRAGMENT %s\n%s\n<<<<<<", m_name.c_str(), fragmentSource.c_str());

    m_shaderProgram->setSourceStrings(fragmentSource, vertexSource);

    if (m_selection) {
        m_selectionProgram->setDescription("selection_program {style:" + m_name + "}");

        initShaderSource(out, false);
        buildVertexShaderSource(out, true);

        m_selectionProgram->setSourceStrings(SHADER_SOURCE(selection_fs),
                                             out.string);
    }
}

void Style::buildMaterialAndLightGlobal(bool _fragment, ShaderSource& out) {

    if (hasRasters()) {
        if (_fragment) {
            out << SHADER_SOURCE(rasters_glsl);
            auto numSources = std::to_string(m_numRasterSources);
            out << "uniform sampler2D u_rasters[" + numSources +"];";
            out << "uniform vec2 u_raster_sizes[" + numSources +"];";
            out << "uniform vec3 u_raster_offsets[" + numSources +"];";
        }
        out << "varying vec4 v_modelpos_base_zoom;";
    }

    if (m_lightingType == LightingType::vertex) {
        out << "varying vec4 v_lighting;";
    }

    bool lighting = _fragment
        ? m_lightingType == LightingType::fragment
        : m_lightingType == LightingType::vertex;

    if (lighting) {
        out << "vec4 light_accumulator_ambient = vec4(0.0);";
        if (m_material.material->hasDiffuse()) {
            out << "vec4 light_accumulator_diffuse = vec4(0.0);";
        }
        if (m_material.material->hasSpecular()) {
            out << "vec4 light_accumulator_specular = vec4(0.0);";
        }
    }

    m_material.material->buildMaterialBlock(out);
    if (_fragment) {
        m_material.material->buildMaterialFragmentBlock(out);
    }

    if (lighting) {
        // (struct definitions and function definitions)
        for (const auto& light : m_lights) {
            light.light->buildClassBlock(*m_material.material, out);
            light.light->buildInstanceBlock(out);
        }

        out << "vec4 calculateLighting(in vec3 _eyeToPoint, in vec3 _normal, in vec4 _color) {";

        if (_fragment) {
            // Do initial material calculations over normal, emission, ambient,
            // diffuse and specular values
            out << "    calculateMaterial(_eyeToPoint, _normal);";
        }

        // Unroll the loop of individual lights to calculate
        for (const auto& light : m_lights) {
            light.light->buildInstanceComputeBlock(out);
        }
        //  Final light intensity calculation
        if (m_material.material->hasEmission()) {
            out << "    vec4 color = material.emission;";
        } else {
            out << "    vec4 color = vec4(0.0);";
        }
        if (m_material.material->hasAmbient()) {
            out << "    color += light_accumulator_ambient * _color * material.ambient;";
        } else if (m_material.material->hasDiffuse()) {
            // FIXME what is this for?
            out << "    color += light_accumulator_ambient * _color * material.diffuse;";
        }
        if (m_material.material->hasDiffuse()) {
            out << "    color += light_accumulator_diffuse * _color * material.diffuse;";
        }
        if (m_material.material->hasSpecular()) {
            out << "    color += light_accumulator_specular * material.specular;";
        }
        // Clamp final color
        out << "    return clamp(color, 0.0, 1.0);";
        out << "}";
    }
}

void Style::buildMaterialAndLightBlock(bool _fragment, ShaderSource& out) {

    out << "    material = u_material;";

    if (hasRasters() && !_fragment) {
        out << "    v_modelpos_base_zoom = modelPositionBaseZoom();";
    }

    bool lighting = _fragment
        ? m_lightingType == LightingType::fragment
        : m_lightingType == LightingType::vertex;

    if (lighting) {
        for (auto& light : m_lights) {
            light.light->buildSetupBlock(out);
        }
    }

    if (!_fragment) {
        if (m_lightingType == LightingType::vertex) {
            // Modify normal before lighting
            out << "    vec3 normal = v_normal;";

            // FIXME shouldn't the modified normal get passed to the fragment shader?
            insertShaderBlock("normal", out);
            // Like this?
            out << "    v_normal = normal;";

            out << "    v_lighting = calculateLighting(v_position.xyz, normal, vec4(1.));";
        }

    } else {
        if (m_rasterType == RasterType::color) {
            out << "    color *= sampleRaster(0);";
        }
        if (m_rasterType == RasterType::normal) {
            out << "    normal = normalize(sampleRaster(0).rgb * 2.0 - 1.0);";
        }

        if (getMaterial()->hasNormalTexture()) {
            out << "    calculateNormal(normal);";
        }
        // Modify normal before lighting if not already modified in vertex shader
        if (m_lightingType != LightingType::vertex) {
            insertShaderBlock("normal", out);
        }
        // Modify color before lighting is applied
        insertShaderBlock("color", out);

        if (m_lightingType == LightingType::fragment) {
            out << "    color = calculateLighting(v_position.xyz, normal, color);";
        } else if (m_lightingType == LightingType::vertex) {
            out << "    color *= v_lighting;";
        }
    }
}

void Style::build(const Scene& _scene) {

    if (m_material.material) {
        m_material.uniforms = m_material.material->getUniforms(*m_shaderProgram);
    }

    if (m_lightingType != LightingType::none) {
        for (auto& light : _scene.lights()) {
            auto uniforms = light->getUniforms(*m_shaderProgram);
            m_lights.emplace_back(light.get(), std::move(uniforms));
        }
    }

    if (hasRasters()) {
        // Determine maximal number of textures that could be bound
        m_numRasterSources = 0;
        for (const auto& dataSource : _scene.dataSources()) {
            if (dataSource->isRaster()) {
                m_numRasterSources++;
            }
        }

        if (m_numRasterSources == 0) {
            LOGW("No valid raster source!");
            m_rasterType = RasterType::none;
        }
    }

    constructVertexLayout();
    constructShaderProgram();
}

bool Style::hasColorShaderBlock() const {
    if (m_numRasterSources > 0) { return true; }

    const auto& blocks = getSourceBlocks();
    if (blocks.find("color") != blocks.end() ||
        blocks.find("filter") != blocks.end()) {
        return true;
    }
    return false;
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
        if (light.uniforms) {
            light.light->setupProgram(rs, _view, *light.uniforms);
        }
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
