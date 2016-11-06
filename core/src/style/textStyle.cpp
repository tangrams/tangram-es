#include "textStyle.h"
#include "textStyleBuilder.h"

#include "gl/dynamicQuadMesh.h"
#include "gl/mesh.h"
#include "gl/renderState.h"
#include "gl/shaderProgram.h"
#include "labels/textLabels.h"
#include "log.h"
#include "text/fontContext.h"
#include "view/view.h"

namespace Tangram {

TextStyle::TextStyle(std::string _name, std::shared_ptr<FontContext> _fontContext,
                     bool _sdf, Blending _blendMode, GLenum _drawMode, bool _selection)
    : Style(_name, _blendMode, _drawMode, _selection), m_sdf(_sdf),
      m_context(_fontContext ? _fontContext : std::make_shared<FontContext>()) {}

TextStyle::~TextStyle() {}

void TextStyle::constructVertexLayout() {
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 2, GL_SHORT, false, 0},
        {"a_uv", 2, GL_UNSIGNED_SHORT, false, 0},
        {"a_selection_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_stroke", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_alpha", 1, GL_UNSIGNED_SHORT, true, 0},
        {"a_scale", 1, GL_UNSIGNED_SHORT, false, 0},
    }));
}

void TextStyle::onBeginUpdate() {

    // Clear vertices from previous frame
    for (auto& mesh : m_meshes) { mesh->clear(); }

    // Ensure that meshes are available to push to on labels::update()
    size_t s = m_context->glyphTextureCount();
    while (m_meshes.size() < s) {
        m_meshes.push_back(std::make_unique<DynamicQuadMesh<TextVertex>>(m_vertexLayout, GL_TRIANGLES));
    }
}

void TextStyle::onBeginFrame(RenderState& rs) {

    // Upload meshes and textures
    m_context->updateTextures(rs);

    for (auto& mesh : m_meshes) { mesh->upload(rs); }
}

void TextStyle::onBeginDrawFrame(RenderState& rs, const View& _view, Scene& _scene) {

    Style::onBeginDrawFrame(rs, _view, _scene);

    auto texUnit = rs.nextAvailableTextureUnit();

    m_shaderProgram->setUniformf(rs, m_mainUniforms.uMaxStrokeWidth,
                                 m_context->maxStrokeWidth());
    m_shaderProgram->setUniformf(rs, m_mainUniforms.uTexScaleFactor,
                                 glm::vec2(1.0f / GlyphTexture::size));
    m_shaderProgram->setUniformi(rs, m_mainUniforms.uTex, texUnit);
    m_shaderProgram->setUniformMatrix4f(rs, m_mainUniforms.uOrtho,
                                        _view.getOrthoViewportMatrix());

    if (m_sdf) {
        m_shaderProgram->setUniformi(rs, m_mainUniforms.uPass, 1);

        for (size_t i = 0; i < m_meshes.size(); i++) {
            if (m_meshes[i]->isReady()) {
                m_context->bindTexture(rs, i, texUnit);
                m_meshes[i]->draw(rs, *m_shaderProgram);
            }
        }
        m_shaderProgram->setUniformi(rs, m_mainUniforms.uPass, 0);
    }

    for (size_t i = 0; i < m_meshes.size(); i++) {
        if (m_meshes[i]->isReady()) {
            m_context->bindTexture(rs, i, texUnit);
            m_meshes[i]->draw(rs, *m_shaderProgram);
        }
    }
}

void TextStyle::onBeginDrawSelectionFrame(RenderState& rs, const View& _view, Scene& _scene) {

    for (auto& mesh : m_meshes) { mesh->upload(rs); }

    Style::onBeginDrawSelectionFrame(rs, _view, _scene);

    m_selectionProgram->setUniformMatrix4f(rs, m_selectionUniforms.uOrtho,
                                           _view.getOrthoViewportMatrix());

    for (const auto& mesh : m_meshes) {
        if (mesh->isReady()) {
            mesh->draw(rs, *m_selectionProgram);
        }
    }
}

std::unique_ptr<StyleBuilder> TextStyle::createBuilder() const {
    return std::make_unique<TextStyleBuilder>(*this);
}


DynamicQuadMesh<TextVertex>& TextStyle::getMesh(size_t id) const {
    if (id >= m_meshes.size()) {
        LOGE("Accesing inconsistent quad mesh");
        assert(false);
        return *m_meshes[0];
    }

    return *m_meshes[id];
}

size_t TextStyle::dynamicMeshSize() const {
    size_t size = 0;
    for (const auto& mesh : m_meshes) {
        size += mesh->bufferSize();
    }

    return size;
}

static const char* s_uniforms = R"(
uniform mat4 u_ortho;
uniform vec4 u_tile_origin;
uniform vec3 u_map_position;
uniform vec2 u_resolution;
uniform float u_time;
uniform float u_meters_per_pixel;
uniform float u_device_pixel_ratio;
uniform vec2 u_uv_scale_factor;
uniform float u_max_stroke_width;
uniform LOWP int u_pass;)";

static const char* s_varyings = R"(
varying vec4 v_color;
varying vec2 v_texcoords;
varying float v_sdf_threshold;
varying float v_sdf_scale;
varying float v_alpha;)";

void TextStyle::buildVertexShaderSource(ShaderSource& out, bool _selectionPass) {

    out << "#define UNPACK_POSITION(x) (x / 4.0)"
        << "#define UNPACK_TEXTURE(x) (x * u_uv_scale_factor)";

    insertShaderBlock("uniforms", out);
    out << s_uniforms;

    out << "attribute vec2 a_uv;"
        << "attribute float a_alpha;"
        << "attribute vec4 a_color;"
        << "attribute vec2 a_position;"
        << "attribute vec4 a_stroke;"
        << "attribute float a_scale;";

    if (_selectionPass) {
        out << "attribute vec4 a_selection_color;";
        out << "varying vec4 v_selection_color;";
    }

    out << s_varyings;

    insertShaderBlock("global", out);

    out << "void main() {"
        << "    v_alpha = a_alpha;"
        << "    v_color = a_color;";

    if (_selectionPass) {
        out << "    v_selection_color = a_selection_color;";
        // Skip non-selectable meshes
        out << "    if (v_selection_color == vec4(0.0)) {";
        out << "        gl_Position = vec4(0.0);";
        out << "        return;";
        out << "    }";
    }
    out << "    vec2 vertex_pos = UNPACK_POSITION(a_position);"
        << "    v_texcoords = UNPACK_TEXTURE(a_uv);"
        << "    v_sdf_scale = a_scale / 64.0;"
        << "    if (u_pass == 0) {"
        // fill
        << "        v_sdf_threshold = 0.5;"
        //v_alpha = 0.0;
        << "    } else if (a_stroke.a > 0.0) {"
        // stroke
        // (0.5 / 3.0) <= sdf change by pixel distance to outline == 0.083
        << "        float sdf_pixel = 0.5/u_max_stroke_width;"
        // de-normalize [0..1] -> [0..max_stroke_width]
        << "        float stroke_width = a_stroke.a * u_max_stroke_width;"
        // scale to sdf pixel
        << "        stroke_width *= sdf_pixel;"
        // scale sdf (texture is scaled depeding on font size)
        << "        stroke_width /= v_sdf_scale;"
        << "        v_sdf_threshold = max(0.5 - stroke_width, 0.0);"
        << "        v_color.rgb = a_stroke.rgb;"
        << "    } else {"
        << "        v_alpha = 0.0;"
        << "    }";

    out << "    vec4 position = vec4(vertex_pos, 0.0, 1.0);";

    insertShaderBlock("position", out);

    out << "    gl_Position = u_ortho * position;";
    out << "}";
}

void TextStyle::buildFragmentShaderSource(ShaderSource& out) {

    insertShaderBlock("uniforms", out);
    out << s_uniforms;
    out << "uniform sampler2D u_tex;";

    out << s_varyings;

    insertShaderBlock("global", out);

    out << "void main(void) {"
        << "    vec4 color = v_color;"
        << "    float signed_distance = texture2D(u_tex, v_texcoords).a;"

        // - At the glyph outline alpha is 0.5
        //
        // - The sdf-radius is 3.0px, i.e. within 3px distance
        //   from the outline alpha is in the range (0.5 -> 0.0)
        //
        // - 0.5 pixel threshold (to both sides of the outline)
        //   plus 0.25 for a bit of smoothness
        //
        //   ==> (0.5 / 3.0) * (0.5 + 0.25) == 0.1245
        //   This value is added to sdf_threshold to antialias
        //   the outline within one pixel for the *unscaled* glyph.
        //
        // - sdf_scale == fontScale / glyphScale:
        //   When the glyph is scaled down, 's' must be increased
        //   (used to interpolate 1px of the scaled glyph around v_sdf_threshold)

        << "    float sdf_pixel = 0.5 / (u_max_stroke_width * v_sdf_scale);"
        << "    float add_smooth = 0.25;"
        << "    float filter_width = (sdf_pixel * (0.5 + add_smooth));"

        << "    float start = max(v_sdf_threshold - filter_width, 0.0);"
        << "    float end = v_sdf_threshold + filter_width;"

        << "    float alpha;"
        << "    if (u_pass == 0) {"
        << "        alpha = smoothstep(start, end, signed_distance);"
        << "    } else {"
        // smooth the signed distance for outlines
        << "        float signed_distance_1_over_2 = 1.0 / (2.0 * signed_distance);"
        << "        float smooth_signed_distance = pow(signed_distance, signed_distance_1_over_2);"
        << "        alpha = smoothstep(start, end, smooth_signed_distance);"
        << "    }";

    out << "    color.a *= v_alpha * alpha;";

    insertShaderBlock("color", out);
    insertShaderBlock("filter", out);

    out << "    gl_FragColor = color;";
    out << "}";

}

}
