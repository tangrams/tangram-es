#include "fontStyle.h"

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

    if (!m_shaderProgram->buildFromSourceStrings(fragShaderSrcStr, vertShaderSrcStr)) {
        logMsg("[FontStyle] Error building text shader program\n");
    }
}

void FontStyle::buildPoint(Point& _point, std::string& _layer, Properties& _props, VboMesh& _mesh) {

}

void FontStyle::buildLine(Line& _line, std::string& _layer, Properties& _props, VboMesh& _mesh) {
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

                glm::dvec2 p1 = glm::dvec2(_line[0]);
                glm::dvec2 p2 = glm::dvec2(_line[1]);
                glm::dvec2 p1p2 = p1 - p2;

                if(glm::length(p1p2) < 0.5) { // some random label discard for now
                    return;
                }

                p1p2 = glm::normalize(p1p2);
                float rot = (float) atan2(p1p2.x, p1p2.y) + M_PI_2;

                double offset = 1;
                if (rot > M_PI_2 || rot < -M_PI_2) {
                    rot += M_PI;
                    offset = -1;
                }

                glm::dvec2 position = (p1 + p2) / 2.0 + p1p2 * 0.2 * offset;

                auto label = labelContainer->addLabel(m_name, { position, 1.0, rot }, prop.second);

                label->rasterize();
            }
        }
    }

    ftContext->clearState();
    
    if (textBuffer->getVertices(&vertData, &nVerts)) {
         _mesh.addVertices((GLbyte*)vertData.data(), nVerts);
    }
}

void FontStyle::buildPolygon(Polygon& _polygon, std::string& _layer, Properties& _props, VboMesh& _mesh) {

}

void FontStyle::prepareDataProcessing(MapTile& _tile) {
    auto ftContext = LabelContainer::GetInstance()->getFontContext();
    auto buffer = ftContext->genTextBuffer();

    _tile.setTextBuffer(*this, buffer);

    ftContext->lock();
    ftContext->useBuffer(buffer);

    buffer->init();

    LabelContainer::GetInstance()->processedTile = &_tile;
}

void FontStyle::finishDataProcessing(MapTile& _tile) {
    auto ftContext = LabelContainer::GetInstance()->getFontContext();
    
    LabelContainer::GetInstance()->processedTile = nullptr;

    ftContext->useBuffer(nullptr);;
    ftContext->unlock();
}

void FontStyle::setupFrame(const std::shared_ptr<View>& _view) {
    auto ftContext = LabelContainer::GetInstance()->getFontContext();
    const auto& atlas = ftContext->getAtlas();
    float projectionMatrix[16];

    ftContext->getViewProjection(projectionMatrix);

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
