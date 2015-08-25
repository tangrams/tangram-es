#include "textStyle.h"

#include "text/fontContext.h"
#include "tile/tile.h"
#include "gl/shaderProgram.h"
#include "gl/vboMesh.h"
#include "view/view.h"
#include "labels/textLabel.h"
#include "glm/gtc/type_ptr.hpp"

namespace Tangram {

const static std::string key_name("name");

TextStyle::TextStyle(std::string _name, GLenum _drawMode, bool _sdf, bool _sdfMultisampling) :
    Style(_name, _drawMode), m_sdf(_sdf), m_sdfMultisampling(_sdfMultisampling) {}

TextStyle::~TextStyle() {
}

void TextStyle::constructVertexLayout() {
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 2, GL_FLOAT, false, 0},
        {"a_uv", 2, GL_FLOAT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_screenPosition", 2, GL_FLOAT, false, 0},
        {"a_alpha", 1, GL_FLOAT, false, 0},
        {"a_rotation", 1, GL_FLOAT, false, 0},
    }));
}

void TextStyle::constructShaderProgram() {
    std::string frag = m_sdf ? "sdf.fs" : "text.fs";

    std::string vertShaderSrcStr = stringFromResource("point.vs");
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

    //TODO: handle different size formats, px, pt, em

    std::string typefaceStr;
    std::string cap;

    _rule.getValue(StyleParamKey::font_typeface, typefaceStr);
    _rule.getColor(StyleParamKey::font_fill, p.fill);
    if (_rule.getColor(StyleParamKey::font_stroke, p.strokeColor)) {
        _rule.getColor(StyleParamKey::font_stroke_color, p.strokeColor);
    }
    _rule.getValue(StyleParamKey::font_stroke_width, p.strokeWidth);
    if (_rule.getValue(StyleParamKey::font_capitalized, cap) ) {
        if (cap == "true") { p.capitalized = true; }
        else { p.capitalized = false; }
    }

    // Parse typefaceStr to Property.typeface and Property.size
    // Atleast for android fontName should be like: Roboto-BoldItalic
    if( parseTypeFaceFontsInfo(typefaceStr, p.fontName, p.fontSize) ) {
    } else {
        logMsg("Error in parsing typeface font information.\n");
    }

    /* Global operations done for fontsize and sdfblur */
    p.fontSize *= m_pixelScale;
    p.blurSpread = m_sdf ? 2.5f : 0.0f;

    return p;
}

void TextStyle::buildPoint(const Point& _point, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    auto& buffer = static_cast<TextBuffer&>(_mesh);

	Parameters params = parseRule(_rule);

    const auto& text = _props.getString(key_name);
    if (text.length() == 0) { return; }

    Label::Options options;
    options.color = params.fill;

    buffer.addLabel(text, { glm::vec2(_point), glm::vec2(_point) }, Label::Type::point, params, options);
}

void TextStyle::buildLine(const Line& _line, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    auto& buffer = static_cast<TextBuffer&>(_mesh);

	Parameters params = parseRule(_rule);

    const auto& text = _props.getString(key_name);
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

        Label::Options options;
        options.color = params.fill;

        buffer.addLabel(text, { p1, p2 }, Label::Type::line, params, options);
    }
}

void TextStyle::buildPolygon(const Polygon& _polygon, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    auto& buffer = static_cast<TextBuffer&>(_mesh);

	Parameters params = parseRule(_rule);

    const auto& text = _props.getString(key_name);
    if (text.length() == 0) { return; }

    glm::vec2 centroid;
    int n = 0;

    for (auto& l : _polygon) {
        for (auto& p : l) {
            centroid.x += p.x;
            centroid.y += p.y;
            n++;
        }
    }
    if (n == 0) { return; }

    centroid /= n;

    Label::Options options;
    options.color = params.fill;

    buffer.addLabel(text, { centroid, centroid }, Label::Type::point, params, options);

}

void TextStyle::onBeginDrawFrame(const View& _view, const Scene& _scene) {
    bool contextLost = Style::glContextLost();

    FontContext::GetInstance()->bindAtlas(0);

    if (contextLost) {
        m_shaderProgram->setUniformi("u_tex", 0);
    }

    if (m_dirtyViewport || contextLost) {
        m_shaderProgram->setUniformf("u_resolution", _view.getWidth(), _view.getHeight());
        m_shaderProgram->setUniformMatrix4f("u_proj", glm::value_ptr(_view.getOrthoViewportMatrix()));
        m_dirtyViewport = false;
    }

    RenderState::blending(GL_TRUE);
    RenderState::blendingFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    RenderState::depthTest(GL_FALSE);
}

}
