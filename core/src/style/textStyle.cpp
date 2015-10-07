#include "textStyle.h"

#include "text/fontContext.h"
#include "tile/tile.h"
#include "gl/shaderProgram.h"
#include "gl/vboMesh.h"
#include "view/view.h"
#include "labels/textLabel.h"
#include "glm/gtc/type_ptr.hpp"
#include "tangram.h"
#include "text/fontContext.h"
#include "data/propertyItem.h" // Include wherever Properties is used!

namespace Tangram {

TextStyle::TextStyle(std::string _name, std::shared_ptr<FontContext> _fontContext, bool _sdf,
                     bool _sdfMultisampling, Blending _blendMode, GLenum _drawMode) :
    Style(_name, _blendMode, _drawMode), m_sdf(_sdf), m_sdfMultisampling(_sdfMultisampling),
    m_fontContext(_fontContext) {
}

TextStyle::~TextStyle() {
}

void TextStyle::constructVertexLayout() {
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 2, GL_FLOAT, false, 0},
        {"a_uv", 2, GL_FLOAT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_stroke", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_screenPosition", 2, GL_FLOAT, false, 0},
        {"a_alpha", 1, GL_FLOAT, false, 0},
        {"a_rotation", 1, GL_FLOAT, false, 0},
    }));
}

void TextStyle::constructShaderProgram() {
    std::string frag = m_sdf ? "shaders/sdf.fs" : "shaders/text.fs";

    std::string vertShaderSrcStr = stringFromFile("shaders/point.vs", PathType::internal);
    std::string fragShaderSrcStr = stringFromFile(frag.c_str(), PathType::internal);

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);

    std::string defines;

    if (m_sdf && m_sdfMultisampling) {
        defines += "#define TANGRAM_SDF_MULTISAMPLING\n";
    }

    m_shaderProgram->addSourceBlock("defines", defines);
}

Parameters TextStyle::applyRule(const DrawRule& _rule, const Properties& _props) const {
    const static std::string key_name("name");

    Parameters p;

    std::string fontFamily, fontWeight, fontStyle, transform;
    glm::vec2 offset;

    _rule.get(StyleParamKey::font_family, fontFamily);
    _rule.get(StyleParamKey::font_weight, fontWeight);
    _rule.get(StyleParamKey::font_style, fontStyle);
    std::string fontKey = fontFamily + "_" + fontWeight + "_" + fontStyle;
    {
        if (!m_fontContext->lock()) { return p; }

        p.fontId = m_fontContext->addFont(fontFamily, fontWeight, fontStyle);

        m_fontContext->unlock();
        if (p.fontId < 0) { return p; }
    }

    _rule.get(StyleParamKey::font_size, p.fontSize);
    _rule.get(StyleParamKey::font_fill, p.fill);
    _rule.get(StyleParamKey::offset, p.offset);
    _rule.get(StyleParamKey::font_stroke_color, p.strokeColor);
    _rule.get(StyleParamKey::font_stroke_width, p.strokeWidth);
    _rule.get(StyleParamKey::transform, transform);
    _rule.get(StyleParamKey::visible, p.visible);
    _rule.get(StyleParamKey::priority, p.priority);

    _rule.get(StyleParamKey::text_source, p.text);
    if (!_rule.isJSFunction(StyleParamKey::text_source)) {
        if (p.text.empty()) {
            p.text = _props.getString(key_name);
        } else {
            p.text = _props.getString(p.text);
        }
    }

   if (_rule.get(StyleParamKey::interactive, p.interactive) && p.interactive) {
       p.properties = std::make_shared<Properties>(_props);
   }

    if (transform == "capitalize") {
        p.transform = TextTransform::capitalize;
    } else if (transform == "lowercase") {
        p.transform = TextTransform::lowercase;
    } else if (transform == "uppercase") {
        p.transform = TextTransform::uppercase;
    }

    /* Global operations done for fontsize and sdfblur */
    float emSize = p.fontSize / 16.f;
    p.fontSize *= m_pixelScale;
    p.strokeWidth *= m_pixelScale;
    p.blurSpread = m_sdf ? emSize * 5.0f : 0.0f;

    return p;
}

void TextStyle::buildPoint(const Point& _point, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    auto& buffer = static_cast<TextBuffer&>(_mesh);

    Parameters params = applyRule(_rule, _props);

    if (!params.visible || !params.isValid()) { return; }

    buffer.addLabel(params, { glm::vec2(_point), glm::vec2(_point) },
                    Label::Type::point, *m_fontContext);
}

void TextStyle::buildLine(const Line& _line, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    auto& buffer = static_cast<TextBuffer&>(_mesh);

    Parameters params = applyRule(_rule, _props);

    if (!params.visible || !params.isValid()) { return; }

    for (size_t i = 0; i < _line.size() - 1; i++) {
        glm::vec2 p1 = glm::vec2(_line[i]);
        glm::vec2 p2 = glm::vec2(_line[i + 1]);

        buffer.addLabel(params, { p1, p2 }, Label::Type::line, *m_fontContext);
    }
}

void TextStyle::buildPolygon(const Polygon& _polygon, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    Point p = glm::vec3(centroid(_polygon), 0.0);
    buildPoint(p, _rule, _props, _mesh, _tile);
}

void TextStyle::onBeginDrawFrame(const View& _view, Scene& _scene) {
    bool contextLost = Style::glContextLost();

    m_fontContext->bindAtlas(0);

    setupShaderUniforms(1, contextLost, _scene);

    if (contextLost) {
        m_shaderProgram->setUniformi("u_tex", 0);
    }

    if (m_dirtyViewport || contextLost) {
        m_shaderProgram->setUniformMatrix4f("u_ortho", glm::value_ptr(_view.getOrthoViewportMatrix()));
        m_dirtyViewport = false;
    }

    Style::onBeginDrawFrame(_view, _scene);

}

}
