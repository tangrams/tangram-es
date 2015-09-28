#include "textStyle.h"

#include "text/fontContext.h"
#include "tile/tile.h"
#include "gl/shaderProgram.h"
#include "gl/vboMesh.h"
#include "view/view.h"
#include "labels/textLabel.h"
#include "glm/gtc/type_ptr.hpp"
#include "tangram.h"

namespace Tangram {

TextStyle::TextStyle(std::string _name, bool _sdf, bool _sdfMultisampling, Blending _blendMode, GLenum _drawMode) :
    Style(_name, _blendMode, _drawMode), m_sdf(_sdf), m_sdfMultisampling(_sdfMultisampling) {
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

    std::string vertShaderSrcStr = stringFromResource("shaders/point.vs");
    std::string fragShaderSrcStr = stringFromResource(frag.c_str());

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);

    std::string defines;

    if (m_sdf && m_sdfMultisampling) {
        defines += "#define TANGRAM_SDF_MULTISAMPLING\n";
    }

    m_shaderProgram->addSourceBlock("defines", defines);
}

Parameters TextStyle::parseRule(const DrawRule& _rule) const {
    Parameters p;

    std::string fontFamily, fontWeight, fontStyle, transform;
    glm::vec2 offset;

    _rule.get(StyleParamKey::font_family, fontFamily);
    _rule.get(StyleParamKey::font_weight, fontWeight);
    _rule.get(StyleParamKey::font_style, fontStyle);
    _rule.get(StyleParamKey::font_size, p.fontSize);
    _rule.get(StyleParamKey::font_fill, p.fill);
    _rule.get(StyleParamKey::offset, p.offset);
    _rule.get(StyleParamKey::font_stroke_color, p.strokeColor);
    _rule.get(StyleParamKey::font_stroke_width, p.strokeWidth);
    _rule.get(StyleParamKey::transform, transform);
    _rule.get(StyleParamKey::visible, p.visible);
    _rule.get(StyleParamKey::priority, p.priority);
    if (_rule.get(StyleParamKey::text_source, p.textSource.text)) {
        p.textSource.isFunction = _rule.isJSFunction(StyleParamKey::text_source);
    }

    if (transform == "capitalize") {
        p.transform = TextTransform::capitalize;
    } else if (transform == "lowercase") {
        p.transform = TextTransform::lowercase;
    } else if (transform == "uppercase") {
        p.transform = TextTransform::uppercase;
    }

    p.fontKey = fontFamily + "_" + fontWeight + "_" + fontStyle;

    /* Global operations done for fontsize and sdfblur */
    float emSize = p.fontSize / 16.f;
    p.fontSize *= m_pixelScale;
    p.strokeWidth *= m_pixelScale;
    p.blurSpread = m_sdf ? emSize * 5.0f : 0.0f;

    return p;
}

Label::Options TextStyle::optionsFromTextParams(const Parameters& _params) const {
    Label::Options options;
    options.color = _params.fill;
    options.priority = _params.priority;
    options.offset = _params.offset * m_pixelScale;
    return options;
}

const std::string& TextStyle::applyTextSource(const Parameters& _parameters, const Properties& _props) const {

    const static std::string key_name("name");

    if (_parameters.textSource.isFunction) {
        return _parameters.textSource.text;
    }

    if (_parameters.textSource.text.empty()) {
        // Default: use 'name' property
        return _props.getString(key_name);
    } else {
        return _props.getString(_parameters.textSource.text);
    }
}

void TextStyle::buildPoint(const Point& _point, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    auto& buffer = static_cast<TextBuffer&>(_mesh);

    Parameters params = parseRule(_rule);

    if (!params.visible) {
        return;
    }

    const std::string& text = applyTextSource(params, _props);

    if (text.length() == 0) { return; }
    buffer.addLabel(text, { glm::vec2(_point), glm::vec2(_point) }, Label::Type::point, params, optionsFromTextParams(params));
}

void TextStyle::buildLine(const Line& _line, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    auto& buffer = static_cast<TextBuffer&>(_mesh);

    Parameters params = parseRule(_rule);

    if (!params.visible) {
        return;
    }

    const std::string& text = applyTextSource(params, _props);

    if (text.length() == 0) { return; }

    int lineLength = _line.size();
    int skipOffset = floor(lineLength / 2);
    float minLength = 0.15; // default, probably need some more thoughts


    for (size_t i = 0; i < _line.size() - 1; i += skipOffset) {
        glm::vec2 p1 = glm::vec2(_line[i]);
        glm::vec2 p2 = glm::vec2(_line[i + 1]);

        glm::vec2 p1p2 = p2 - p1;
        float length = glm::length(p1p2);

        if (length < minLength) {
            continue;
        }

        buffer.addLabel(text, { p1, p2 }, Label::Type::line, params, optionsFromTextParams(params));
    }

}

void TextStyle::buildPolygon(const Polygon& _polygon, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    auto& buffer = static_cast<TextBuffer&>(_mesh);

    Parameters params = parseRule(_rule);

    if (!params.visible) {
        return;
    }

    const std::string& text = applyTextSource(params, _props);

    if (text.length() == 0) { return; }

    glm::vec2 c = centroid(_polygon);

    buffer.addLabel(text, { c, c }, Label::Type::point, params, optionsFromTextParams(params));
}

void TextStyle::onBeginDrawFrame(const View& _view, Scene& _scene) {
    bool contextLost = Style::glContextLost();

    FontContext::GetInstance()->bindAtlas(0);

    setupShaderUniforms(1, contextLost, _scene);

    if (contextLost) {
        m_shaderProgram->setUniformi("u_tex", 0);
    }

    if (m_dirtyViewport || contextLost) {
        m_shaderProgram->setUniformMatrix4f("u_proj", glm::value_ptr(_view.getOrthoViewportMatrix()));
        m_dirtyViewport = false;
    }

    Style::onBeginDrawFrame(_view, _scene);

}

}
