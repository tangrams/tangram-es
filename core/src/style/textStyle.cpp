#include "textStyle.h"

#include "text/fontContext.h"
#include "tile/tile.h"
#include "gl/shaderProgram.h"
#include "gl/vboMesh.h"
#include "view/view.h"
#include "labels/textLabel.h"
#include "glm/gtc/type_ptr.hpp"

namespace Tangram {

TextStyle::TextStyle(const std::string& _fontName, std::string _name, float _fontSize, unsigned int _color, bool _sdf, bool _sdfMultisampling, GLenum _drawMode)
: Style(_name, _drawMode), m_fontName(_fontName), m_fontSize(_fontSize), m_color(_color), m_sdf(_sdf), m_sdfMultisampling(_sdfMultisampling)  {
}

TextStyle::~TextStyle() {
}

void TextStyle::constructVertexLayout() {
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 2, GL_FLOAT, false, 0},
        {"a_uv", 2, GL_FLOAT, false, 0},
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

void TextStyle::buildPoint(const Point& _point, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    auto& buffer = static_cast<TextBuffer&>(_mesh);

    std::string text;
    if (!_props.getString("name", text)) {
        return;
    }

    Label::Transform t { glm::vec2(_point), glm::vec2(_point), glm::vec2(0) };
    addTextLabel(buffer, t, text, Label::Type::point);
}

void TextStyle::buildLine(const Line& _line, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    auto& buffer = static_cast<TextBuffer&>(_mesh);

    std::string text;
    if (!_props.getString("name", text)) {
        return;
    }

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

        addTextLabel(buffer, { p1, p2, glm::vec2(0) }, text, Label::Type::line);
    }
}

void TextStyle::buildPolygon(const Polygon& _polygon, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {
    auto& buffer = static_cast<TextBuffer&>(_mesh);

    std::string text;
    if (!_props.getString("name", text)) {
        return;
    }

    glm::vec2 centroid;
    int n = 0;

    for (auto& l : _polygon) {
        for (auto& p : l) {
            centroid.x += p.x;
            centroid.y += p.y;
            n++;
        }
    }

    centroid /= n;

    Label::Transform t { centroid, centroid, glm::vec2(0) };

    addTextLabel(buffer, t, text, Label::Type::point);
}

void TextStyle::addTextLabel(TextBuffer& _buffer, Label::Transform _transform, std::string _text, Label::Type _type) const {

    std::unique_ptr<TextLabel> label(new TextLabel(_buffer, _transform, _text, _type));

    if (label->rasterize()) {
        _buffer.addLabel(std::move(label));
    }
}

void TextStyle::onBeginBuildTile(Tile& _tile) const {
    auto& mesh = _tile.getMesh(*this);
    if (!mesh) {
        mesh.reset(newMesh());
    }
    auto& buffer = static_cast<TextBuffer&>(*mesh);
    auto ftContext = FontContext::GetInstance();
    auto font = ftContext->getFontID(m_fontName);

    buffer.init(font, m_fontSize * m_pixelScale, m_sdf ? 2.5 : 0);
}

void TextStyle::onEndBuildTile(Tile& _tile) const {

    auto& mesh = _tile.getMesh(*this);

    auto& buffer = static_cast<TextBuffer&>(*mesh);

    buffer.addBufferVerticesToMesh();
}

void TextStyle::setColor(unsigned int _color) {
    m_color = _color;
    m_dirtyColor = true;
}

void TextStyle::onBeginDrawFrame(const View& _view, const Scene& _scene) {

    FontContext::GetInstance()->bindAtlas(0);

    static bool initUniformSampler = true;

    if (initUniformSampler) {
        m_shaderProgram->setUniformi("u_tex", 0);
        initUniformSampler = false;
    }

    if (true || m_dirtyViewport) {
        m_shaderProgram->setUniformf("u_resolution", _view.getWidth(), _view.getHeight());
        m_shaderProgram->setUniformMatrix4f("u_proj", glm::value_ptr(_view.getOrthoViewportMatrix()));
        m_dirtyViewport = false;
    }

    if (true || m_dirtyColor) {
        float r = (m_color >> 16 & 0xff) / 255.0;
        float g = (m_color >> 8  & 0xff) / 255.0;
        float b = (m_color       & 0xff) / 255.0;

        m_shaderProgram->setUniformf("u_color", r, g, b);
        m_dirtyColor = false;
    }

    RenderState::blending(GL_TRUE);
    RenderState::blendingFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
    RenderState::depthTest(GL_FALSE);
}

}
