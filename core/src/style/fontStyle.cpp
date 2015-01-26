#include "fontStyle.h"
#define GLFONTSTASH_IMPLEMENTATION
#include "glfontstash.h"

FontStyle::FontStyle(const std::string& _fontFile, std::string _name, float _fontSize, bool _sdf, GLenum _drawMode)
: Style(_name, _drawMode), m_fontSize(_fontSize), m_sdf(_sdf) {

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

    // TODO : move to font context
    // fonsSetSize(m_fontContext->m_fsContext, m_fontSize * m_pixelScale);
    // fonsSetFont(m_fontContext->m_fsContext, m_font);

    // if(m_sdf) {
    //     float blurSpread = 2.5;
    //     fonsSetBlur(m_fontContext->m_fsContext, blurSpread);
    //     fonsSetBlurType(m_fontContext->m_fsContext, FONS_EFFECT_DISTANCE_FIELD);
    // }

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
                float r = (float) atan2(p1p2.x, p1p2.y) + M_PI_2;

                double offset = 1;
                if (r > M_PI_2 || r < -M_PI_2) {
                    r += M_PI;
                    offset = -1;
                }

                // TODO : ask label manager to generate a label in current text buffer, and add it
                // fsuint textId;
                // glfonsGenText(m_fontContext->m_fsContext, 1, &textId);

                // glm::dvec2 position = (p1 + p2) / 2.0 + p1p2 * 0.2 * offset;

                // // FUTURE: label logic
                // // 1. project label on screen and compute its bbox, to do so we need the view and label model matrix
                // // 2. ask any kind of label manager to perform label discards

                // std::unique_ptr<Label> label(new Label {
                //     m_fontContext,
                //     textId,
                //     prop.second.c_str(),
                //     position,   // world position
                //     1.0,        // alpha
                //     r           // rotation
                // });

                // if (m_processedTile->addLabel(*this, std::move(label))) { // could potentially refuse to add label

                //     logMsg("[FontStyle] Rasterize label: %s, angle: %f\n", prop.second.c_str(), r * (180/M_PI));

                //     glfonsRasterize(m_fontContext->m_fsContext, textId, prop.second.c_str(), FONS_EFFECT_NONE);
                // }
            }
        }
    }

    // fonsClearState(m_fontContext->m_fsContext);

    // TODO : ask font context to generate those vertices for currently bound text buffer
    // if (glfonsVertices(m_fontContext->m_fsContext, &vertData, &nVerts)) {
    //     _mesh.addVertices((GLbyte*)vertData.data(), nVerts);
    // }
}

void FontStyle::buildPolygon(Polygon& _polygon, std::string& _layer, Properties& _props, VboMesh& _mesh) {

}

void FontStyle::prepareDataProcessing(MapTile& _tile) {
    // TODO : generate a text buffer from font context
    // m_fontContext->m_contextMutex->lock();

    // m_processedTile = &_tile;

    // // start naively with a transform texture of size 2x4, asumption can be done querying the props
    // int transformTextureSize = 2;
    // fsuint buffer = _tile.createTextBuffer(*this, m_fontContext, transformTextureSize);
    // glfonsBindBuffer(m_fontContext->m_fsContext, buffer);
}

void FontStyle::finishDataProcessing(MapTile& _tile) {
    // TODO : unbind the text buffer
    // glfonsBindBuffer(m_fontContext->m_fsContext, 0);

    // m_processedTile = nullptr;

    // m_fontContext->m_contextMutex->unlock();
}

void FontStyle::setupFrame(const std::shared_ptr<View>& _view) {
    // TODO : ask font context for atlas and projection matrix
    // float projectionMatrix[16] = {0};

    // glfonsProjection(m_fontContext->m_fsContext, projectionMatrix);

    // m_atlas->update();
    // m_atlas->bind();

    // m_shaderProgram->setUniformi("u_tex", m_atlas->getTextureSlot()); // atlas
    m_shaderProgram->setUniformf("u_resolution", _view->getWidth(), _view->getHeight());
    m_shaderProgram->setUniformf("u_color", 1.0, 1.0, 1.0);
    // m_shaderProgram->setUniformMatrix4f("u_proj", projectionMatrix);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
}

void FontStyle::teardown() {
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

