#include "pointStyle.h"

#include "gl/dynamicQuadMesh.h"
#include "gl/shaderProgram.h"
#include "gl/texture.h"
#include "gl/vertexLayout.h"
#include "scene/spriteAtlas.h"
#include "style/pointStyleBuilder.h"
#include "view/view.h"

namespace Tangram {

PointStyle::PointStyle(std::string _name, std::shared_ptr<FontContext> _fontContext,
                       Blending _blendMode, GLenum _drawMode, bool _selection)
    : Style(_name, _blendMode, _drawMode, _selection) {

    m_textStyle = std::make_unique<TextStyle>(_name, _fontContext, true, _blendMode, _drawMode);
}

PointStyle::~PointStyle() {}

void PointStyle::constructVertexLayout() {

    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 3, GL_FLOAT, false, 0},
        {"a_uv", 2, GL_UNSIGNED_SHORT, true, 0},
        {"a_selection_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_alpha", 1, GL_UNSIGNED_SHORT, true, 0},
        {"a_scale", 1, GL_UNSIGNED_SHORT, false, 0},
    }));

    m_textStyle->constructVertexLayout();
}

void PointStyle::constructShaderProgram() {

    m_mesh = std::make_unique<DynamicQuadMesh<SpriteVertex>>(m_vertexLayout, m_drawMode);

    m_textStyle->constructShaderProgram();

    Style::constructShaderProgram();
}

void PointStyle::onBeginUpdate() {
    m_mesh->clear();
    m_textStyle->onBeginUpdate();
}

void PointStyle::onBeginFrame(RenderState& rs) {
    // Upload meshes for next frame
    m_mesh->upload(rs);
    m_textStyle->onBeginFrame(rs);
}

void PointStyle::onBeginDrawFrame(RenderState& rs, const View& _view, Scene& _scene) {
    Style::onBeginDrawFrame(rs, _view, _scene);

    auto texUnit = rs.nextAvailableTextureUnit();

    m_shaderProgram->setUniformi(rs, m_mainUniforms.uTex, texUnit);
    m_shaderProgram->setUniformMatrix4f(rs, m_mainUniforms.uOrtho,
                                        _view.getOrthoViewportMatrix());

    m_mesh->draw(rs, *m_shaderProgram, texUnit);

    m_textStyle->onBeginDrawFrame(rs, _view, _scene);
}

void PointStyle::onBeginDrawSelectionFrame(RenderState& rs, const View& _view, Scene& _scene) {
    if (!m_selection) { return; }

    m_mesh->upload(rs);

    Style::onBeginDrawSelectionFrame(rs, _view, _scene);

    m_selectionProgram->setUniformMatrix4f(rs, m_selectionUniforms.uOrtho,
                                           _view.getOrthoViewportMatrix());

    m_mesh->draw(rs, *m_selectionProgram, false);

    m_textStyle->onBeginDrawSelectionFrame(rs, _view, _scene);
}

std::unique_ptr<StyleBuilder> PointStyle::createBuilder() const {
    return std::make_unique<PointStyleBuilder>(*this);
}

void PointStyle::setPixelScale(float _pixelScale) {
    Style::setPixelScale(_pixelScale);
    m_textStyle->setPixelScale(_pixelScale);
}

static const char* s_uniforms = R"(
uniform mat4 u_ortho;
uniform vec4 u_tile_origin;
uniform vec3 u_map_position;
uniform vec2 u_resolution;
uniform float u_time;
uniform float u_meters_per_pixel;
uniform float u_device_pixel_ratio;)";

static const char* s_varyings = R"(
varying vec4 v_color;
varying vec2 v_texcoords;
varying float v_alpha;)";

void PointStyle::buildVertexShaderSource(ShaderSource& out, bool _selectionPass) {

    insertShaderBlock("uniforms", out);
    out << s_uniforms;

    out << "attribute vec2 a_uv;";
    out << "attribute LOWP float a_alpha;";
    out << "attribute LOWP vec4 a_color;";
    out << "attribute vec3 a_position;";

    if (_selectionPass) {
        out << "attribute vec4 a_selection_color;";
        out << "varying vec4 v_selection_color;";
    }

    out << s_varyings;

    insertShaderBlock("global", out);

    out << "void main() {";
    out << "    v_alpha = a_alpha;";
    out << "    v_color = a_color;";

    if (_selectionPass) {
        out << "    v_selection_color = a_selection_color;";
        // Skip non-selectable meshes
        out << "    if (v_selection_color == vec4(0.0)) {";
        out << "        gl_Position = vec4(0.0);";
        out << "        return;";
        out << "    }";
    }

    out << "    v_texcoords = a_uv;";
    out << "    gl_Position = vec4(a_position, 1.0);";
    out << "}";
}

void PointStyle::buildFragmentShaderSource(ShaderSource& out) {

    insertShaderBlock("uniforms", out);
    out << s_uniforms;
    out << "uniform sampler2D u_tex;";

    out << s_varyings;

    insertShaderBlock("global", out);

    out << "void main(void) {";
    out << "    vec4 texColor = texture2D(u_tex, v_texcoords);";
    out << "    vec4 color = vec4(texColor.rgb * v_color.rgb, v_alpha * texColor.a * v_color.a);";

    insertShaderBlock("color", out);
    insertShaderBlock("filter", out);

    out << "    gl_FragColor = color;";
    out << "}";

}

}
