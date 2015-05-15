#include "textStyle.h"

MapTile* TextStyle::processedTile = nullptr;

TextStyle::TextStyle(const std::string& _fontName, std::string _name, float _fontSize, unsigned int _color, bool _sdf, bool _sdfMultisampling, GLenum _drawMode)
: Style(_name, _drawMode), m_fontName(_fontName), m_fontSize(_fontSize), m_color(_color), m_sdf(_sdf), m_sdfMultisampling(_sdfMultisampling)  {

    constructVertexLayout();
    constructShaderProgram();
}

TextStyle::~TextStyle() {
}

void TextStyle::constructVertexLayout() {
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 2, GL_FLOAT, false, 0},
        {"a_texCoord", 2, GL_FLOAT, false, 0},
        {"a_fsid", 1, GL_FLOAT, false, 0},
    }));
}

void TextStyle::constructShaderProgram() {
    std::string frag = m_sdf ? "sdf.fs" : "text.fs";

    std::string vertShaderSrcStr = stringFromResource("text.vs");
    std::string fragShaderSrcStr = stringFromResource(frag.c_str());
    
    m_shaderProgram = std::make_shared<ShaderProgram>();
    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);
    
    std::string defines;
    
    if (m_sdf && m_sdfMultisampling) {
        defines += "#define TANGRAM_SDF_MULTISAMPLING\n";
    }
    
    m_shaderProgram->addSourceBlock("defines", defines);
}

void TextStyle::buildPoint(Point& _point, std::string& _layer, Properties& _props, VboMesh& _mesh) const {
    std::vector<PosTexID> vertices;
    auto labelContainer = LabelContainer::GetInstance();
    auto ftContext = labelContainer->getFontContext();
    auto textBuffer = ftContext->getCurrentBuffer();
    
    if (!textBuffer) {
        return;
    }
    
    ftContext->setFont(m_fontName, m_fontSize * m_pixelScale);
    
    if (m_sdf) {
        float blurSpread = 2.5;
        ftContext->setSignedDistanceField(blurSpread);
    }
    
    if (_layer == "pois") {
        for (auto prop : _props.stringProps) {
            if (prop.first == "name") {
                labelContainer->addLabel(TextStyle::processedTile->getID(), m_name, { glm::vec2(_point), glm::vec2(_point) }, prop.second, Label::Type::POINT,
                                         processedTile->getModelMatrix());
            }
        }
    }
    
    ftContext->clearState();
    
    vertices.resize(textBuffer->getVerticesSize());
    
    if (textBuffer->getVertices(reinterpret_cast<float*>(vertices.data()))) {
        auto& mesh = static_cast<TextStyle::Mesh&>(_mesh);
        mesh.addVertices(std::move(vertices), {});
    }

}

void TextStyle::buildLine(Line& _line, std::string& _layer, Properties& _props, VboMesh& _mesh) const {
    std::vector<PosTexID> vertices;
    auto labelContainer = LabelContainer::GetInstance();
    auto ftContext = labelContainer->getFontContext();
    auto textBuffer = ftContext->getCurrentBuffer();

    if (!textBuffer) {
        return;
    }

    ftContext->setFont(m_fontName, m_fontSize * m_pixelScale);

    if (m_sdf) {
        float blurSpread = 2.5;
        ftContext->setSignedDistanceField(blurSpread);
    }

    int lineLength = _line.size();
    int skipOffset = floor(lineLength / 2);
    float minLength = 0.15; // default, probably need some more thoughts
    
    if (_layer == "roads") {
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

                    labelContainer->addLabel(TextStyle::processedTile->getID(), m_name, { p1, p2 }, prop.second,
                                             Label::Type::LINE, processedTile->getModelMatrix());
                }
            }
        }
    }

    ftContext->clearState();
    
    vertices.resize(textBuffer->getVerticesSize());
    
    if (textBuffer->getVertices(reinterpret_cast<float*>(vertices.data()))) {
        auto& mesh = static_cast<TextStyle::Mesh&>(_mesh);
        mesh.addVertices(std::move(vertices), {});
    }
}

void TextStyle::buildPolygon(Polygon& _polygon, std::string& _layer, Properties& _props, VboMesh& _mesh) const {
    
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
    
    std::vector<PosTexID> vertices;
    auto labelContainer = LabelContainer::GetInstance();
    auto ftContext = labelContainer->getFontContext();
    auto textBuffer = ftContext->getCurrentBuffer();
    
    if (!textBuffer) {
        return;
    }
    
    ftContext->setFont(m_fontName, m_fontSize * m_pixelScale);
    
    if (m_sdf) {
        float blurSpread = 2.5;
        ftContext->setSignedDistanceField(blurSpread);
    }

    for (auto prop : _props.stringProps) {
        if (prop.first == "name") {
            labelContainer->addLabel(TextStyle::processedTile->getID(), m_name, { glm::vec2(centroid), glm::vec2(centroid) }, prop.second,
                                     Label::Type::POINT, processedTile->getModelMatrix());
        }
    }
    
    ftContext->clearState();
    
    vertices.resize(textBuffer->getVerticesSize());
    
    if (textBuffer->getVertices(reinterpret_cast<float*>(vertices.data()))) {
        auto& mesh = static_cast<TextStyle::Mesh&>(_mesh);
        mesh.addVertices(std::move(vertices), {});
    }
}

void TextStyle::prepareDataProcessing(MapTile& _tile) const {
    auto ftContext = LabelContainer::GetInstance()->getFontContext();
    auto buffer = ftContext->genTextBuffer();

    _tile.setTextBuffer(*this, buffer);

    ftContext->lock();
    ftContext->useBuffer(buffer);

    buffer->init();

    TextStyle::processedTile = &_tile;
}

void TextStyle::finishDataProcessing(MapTile& _tile) const {
    auto ftContext = LabelContainer::GetInstance()->getFontContext();

    TextStyle::processedTile = nullptr;

    ftContext->useBuffer(nullptr);
    ftContext->unlock();
}

void TextStyle::setupTile(const std::shared_ptr<MapTile>& _tile) {
    auto buffer = _tile->getTextBuffer(*this);

    if (buffer) {
        auto texture = buffer->getTextureTransform();

        if (texture) {
            texture->update(0);
            texture->bind(0);
            m_shaderProgram->setUniformi("u_transforms", 0);
            // resolution of the transform texture
            m_shaderProgram->setUniformf("u_tresolution", texture->getWidth(), texture->getHeight());
        }
    }
}

void TextStyle::setupFrame(const std::shared_ptr<View>& _view, const std::shared_ptr<Scene>& _scene) {
    auto ftContext = LabelContainer::GetInstance()->getFontContext();
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

void TextStyle::teardown() {
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
