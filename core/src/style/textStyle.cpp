#include "textStyle.h"

#include "text/fontContext.h"
#include "tile/mapTile.h"
#include "util/shaderProgram.h"
#include "util/vboMesh.h"
#include "view/view.h"

MapTile* TextStyle::s_processedTile = nullptr;

TextStyle::TextStyle(const std::string& _fontName, std::string _name, float _fontSize, unsigned int _color, bool _sdf, bool _sdfMultisampling, GLenum _drawMode)
: Style(_name, _drawMode), m_fontName(_fontName), m_fontSize(_fontSize), m_color(_color), m_sdf(_sdf), m_sdfMultisampling(_sdfMultisampling)  {    
    m_labels = Labels::GetInstance();
}

TextStyle::~TextStyle() {
}

void TextStyle::constructVertexLayout() {
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 2, GL_FLOAT, false, 0},
        {"a_uvs", 2, GL_FLOAT, false, 0},
        {"a_screenPosition", 2, GL_FLOAT, false, 0},
        {"a_alpha", 1, GL_FLOAT, false, 0},
        {"a_rotation", 1, GL_FLOAT, false, 0},
    }));
}

void TextStyle::constructShaderProgram() {
    std::string frag = m_sdf ? "sdf.fs" : "text.fs";

    std::string vertShaderSrcStr = stringFromResource("text.vs");
    std::string fragShaderSrcStr = stringFromResource(frag.c_str());

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);

    std::string defines;

    if (m_sdf && m_sdfMultisampling) {
        defines += "#define TANGRAM_SDF_MULTISAMPLING\n";
    }

    m_shaderProgram->addSourceBlock("defines", defines);
}

void* TextStyle::parseStyleParams(const std::string& _layerNameID, const StyleParamMap& _styleParamMap) {
    return nullptr;
}

void TextStyle::addVertices(TextBuffer& _buffer, VboMesh& _mesh) const {
    std::vector<TextVert> vertices;
    int bufferSize = _buffer.getVerticesSize();
    
    if (bufferSize == 0) {
        return;
    }
    
    vertices.resize(bufferSize);
    
    if (_buffer.getVertices(reinterpret_cast<float*>(vertices.data()))) {
        auto& mesh = static_cast<TextStyle::Mesh&>(_mesh);
        mesh.addVertices(std::move(vertices), {});
    }
}

void TextStyle::buildPoint(Point& _point, void* _styleParams, Properties& _props, VboMesh& _mesh) const {
    for (auto prop : _props.stringProps) {
        if (prop.first == "name") {
            m_labels->addLabel(*TextStyle::s_processedTile, m_name, { glm::vec2(_point), glm::vec2(_point) }, prop.second, Label::Type::POINT);
        }
    }
}

void TextStyle::buildLine(Line& _line, void* _styleParams, Properties& _props, VboMesh& _mesh) const {
    int lineLength = _line.size();
    int skipOffset = floor(lineLength / 2);
    float minLength = 0.15; // default, probably need some more thoughts
    
    for (auto prop : _props.stringProps) {
        if (prop.first.compare("name") == 0) {
            
            for (size_t i = 0; i < _line.size() - 1; i += skipOffset) {
                glm::vec2 p1 = glm::vec2(_line[i]);
                glm::vec2 p2 = glm::vec2(_line[i + 1]);
                
                glm::vec2 p1p2 = p2 - p1;
                float length = glm::length(p1p2);
                
                if (length < minLength) {
                    continue;
                }
                
                m_labels->addLabel(*TextStyle::s_processedTile, m_name, { p1, p2 }, prop.second, Label::Type::LINE);
            }
        }
    }
}

void TextStyle::buildPolygon(Polygon& _polygon, void* _styleParams, Properties& _props, VboMesh& _mesh) const {
    glm::vec3 centroid;
    int n = 0;

    for (auto& l : _polygon) {
        for (auto& p : l) {
            centroid.x += p.x;
            centroid.y += p.y;
            n++;
        }
    }

    centroid /= n;

    for (auto prop : _props.stringProps) {
        if (prop.first == "name") {
            m_labels->addLabel(*TextStyle::s_processedTile, m_name, { glm::vec2(centroid), glm::vec2(centroid) }, prop.second, Label::Type::POINT);
        }
    }
}

void TextStyle::onBeginBuildTile(MapTile& _tile) const {
    auto ftContext = m_labels->getFontContext();
    auto buffer = ftContext->genTextBuffer();

    _tile.setTextBuffer(*this, buffer);

    ftContext->lock();
    ftContext->useBuffer(buffer);

    buffer->init();
    
    ftContext->setFont(m_fontName, m_fontSize * m_pixelScale);
    
    if (m_sdf) {
        float blurSpread = 2.5;
        ftContext->setSignedDistanceField(blurSpread);
    }

    TextStyle::s_processedTile = &_tile;
}

void TextStyle::onEndBuildTile(MapTile &_tile, std::shared_ptr<VboMesh> _mesh) const {
    auto ftContext = m_labels->getFontContext();
    auto buffer = ftContext->getCurrentBuffer();
    
    // add the computed glyph vertices to the mesh once
    addVertices(*ftContext->getCurrentBuffer(), *_mesh);

    buffer->setMesh(_mesh->numVertices() > 0 ? _mesh : nullptr);
    
    TextStyle::s_processedTile = nullptr;
    
    ftContext->clearState();
    ftContext->useBuffer(nullptr);
    ftContext->unlock();
}

void TextStyle::onBeginDrawFrame(const std::shared_ptr<View>& _view, const std::shared_ptr<Scene>& _scene) {
    auto ftContext = m_labels->getFontContext();
    const auto& atlas = ftContext->getAtlas();
    float projectionMatrix[16] = {0};

    ftContext->setScreenSize(_view->getWidth(), _view->getHeight());
    ftContext->getProjection(projectionMatrix);

    atlas->update(1);
    atlas->bind(1);
    
    m_shaderProgram->setUniformi("u_tex", 1);
    m_shaderProgram->setUniformf("u_resolution", _view->getWidth(), _view->getHeight());

    float r = (m_color >> 16 & 0xff) / 255.0;
    float g = (m_color >> 8  & 0xff) / 255.0;
    float b = (m_color       & 0xff) / 255.0;

    m_shaderProgram->setUniformf("u_color", r, g, b);
    m_shaderProgram->setUniformMatrix4f("u_proj", projectionMatrix);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
}

void TextStyle::onEndDrawFrame() {
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
