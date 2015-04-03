#include "fontStyle.h"

MapTile* FontStyle::processedTile = nullptr;

FontStyle::FontStyle(const std::string& _fontName, std::string _name, float _fontSize, bool _sdf, GLenum _drawMode)
: Style(_name, _drawMode), m_fontName(_fontName), m_fontSize(_fontSize), m_sdf(_sdf) {

    constructVertexLayout();
    constructShaderProgram();
}

FontStyle::~FontStyle() {
}

void FontStyle::constructVertexLayout() {
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 2, GL_FLOAT, false, 0},
        {"a_texCoord", 2, GL_FLOAT, false, 0},
        {"a_fsid", 1, GL_FLOAT, false, 0},
    }));
}

void FontStyle::constructShaderProgram() {
    std::string frag = m_sdf ? "sdf.fs" : "text.fs";

    std::string vertShaderSrcStr = stringFromResource("text.vs");
    std::string fragShaderSrcStr = stringFromResource(frag.c_str());

    m_shaderProgram = std::make_shared<ShaderProgram>();
    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);
}

void FontStyle::buildPoint(Point& _point, std::string& _layer, Properties& _props, VboMesh& _mesh) const {

}

void FontStyle::buildLine(Line& _line, std::string& _layer, Properties& _props, VboMesh& _mesh) const {
    std::vector<float> vertData;
    int nVerts = 0;
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

    if (_layer.compare("roads") == 0) {
        for (auto prop : _props.stringProps) {
            if (prop.first.compare("name") == 0 && _line.size() == 2) { // don't treat polylines

                glm::vec2 p1 = glm::vec2(_line[0]);
                glm::vec2 p2 = glm::vec2(_line[1]);
                glm::vec2 p1p2 = p1 - p2;

                if(glm::length(p1p2) < 0.5) { // some random label discard for now
                    return;
                }

                auto label = labelContainer->addLabel(FontStyle::processedTile->getID(), m_name, { p1, p2, 1.0f }, prop.second);

                label->rasterize();
            }
        }
    }

    ftContext->clearState();

    if (textBuffer->getVertices(&vertData, &nVerts)) {
          auto& mesh = static_cast<RawVboMesh&>(_mesh);
          mesh.addVertices((GLbyte*)vertData.data(), nVerts);
    }
}

void FontStyle::buildPolygon(Polygon& _polygon, std::string& _layer, Properties& _props, VboMesh& _mesh) const {

}

void FontStyle::prepareDataProcessing(MapTile& _tile) const {
    auto ftContext = LabelContainer::GetInstance()->getFontContext();
    auto buffer = ftContext->genTextBuffer();

    _tile.setTextBuffer(*this, buffer);

    ftContext->lock();
    ftContext->useBuffer(buffer);

    buffer->init();

    FontStyle::processedTile = &_tile;
}

void FontStyle::finishDataProcessing(MapTile& _tile) const {
    auto ftContext = LabelContainer::GetInstance()->getFontContext();

    FontStyle::processedTile = nullptr;

    ftContext->useBuffer(nullptr);
    ftContext->unlock();
}

void FontStyle::setupTile(const std::shared_ptr<MapTile>& _tile) {
    auto buffer = _tile->getTextBuffer(*this);

    if (buffer) {
        auto texture = buffer->getTextureTransform();

        if (texture) {
            texture->update();
            texture->bind();

            // transform texture
            m_shaderProgram->setUniformi("u_transforms", texture->getTextureSlot());
            // resolution of the transform texture
            m_shaderProgram->setUniformf("u_tresolution", texture->getWidth(), texture->getHeight());
        }
    }
}

void FontStyle::setupFrame(const std::shared_ptr<View>& _view, const std::shared_ptr<Scene>& _scene) {
    auto ftContext = LabelContainer::GetInstance()->getFontContext();
    const auto& atlas = ftContext->getAtlas();
    float projectionMatrix[16] = {0};

    ftContext->setScreenSize(_view->getWidth(), _view->getHeight());
    ftContext->getProjection(projectionMatrix);

    atlas->update();
    atlas->bind();

    m_shaderProgram->setUniformi("u_tex", atlas->getTextureSlot());
    m_shaderProgram->setUniformf("u_resolution", _view->getWidth(), _view->getHeight());
    m_shaderProgram->setUniformf("u_color", 1.0, 1.0, 1.0);
    m_shaderProgram->setUniformMatrix4f("u_proj", projectionMatrix);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
}

void FontStyle::teardown() {
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
