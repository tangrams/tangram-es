#include "fontStyle.h"
#define GLFONTSTASH_IMPLEMENTATION
#include "glfontstash.h"

FontStyle::FontStyle(const std::string& _fontFile, std::string _name, float _fontSize, bool _sdf, GLenum _drawMode)
: Style(_name, _drawMode), m_fontSize(_fontSize), m_sdf(_sdf) {

    constructVertexLayout();
    constructShaderProgram();
    initFontContext(_fontFile);
}

FontStyle::~FontStyle() {
    glfonsDelete(m_fontContext->m_fsContext);

    for (auto& pair : m_transformTileTextures) {
        pair.second->destroy();
    }
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

    fonsSetSize(m_fontContext->m_fsContext, m_fontSize * m_pixelScale);
    fonsSetFont(m_fontContext->m_fsContext, m_font);

    if(m_sdf) {
        float blurSpread = 2.5;
        fonsSetBlur(m_fontContext->m_fsContext, blurSpread);
        fonsSetBlurType(m_fontContext->m_fsContext, FONS_EFFECT_DISTANCE_FIELD);
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
                float r = (float) atan2(p1p2.x, p1p2.y) + M_PI_2;

                double offset = 1;
                if (r > M_PI_2 || r < -M_PI_2) {
                    r += M_PI;
                    offset = -1;
                }

                fsuint textId;
                glfonsGenText(m_fontContext->m_fsContext, 1, &textId);

                glm::dvec2 position = (p1 + p2) / 2.0 + p1p2 * 0.2 * offset;

                // FUTURE: label logic
                // 1. project label on screen and compute its bbox, to do so we need the view and label model matrix
                // 2. ask any kind of label manager to perform label discards

                std::unique_ptr<Label> label(new Label {
                    m_fontContext,
                    textId,
                    prop.second.c_str(),
                    position,   // world position
                    1.0,        // alpha
                    r           // rotation
                });

                if (m_processedTile->addLabel(*this, std::move(label))) { // could potentially refuse to add label

                    logMsg("[FontStyle] Rasterize label: %s, angle: %f\n", prop.second.c_str(), r * (180/M_PI));

                    glfonsRasterize(m_fontContext->m_fsContext, textId, prop.second.c_str(), FONS_EFFECT_NONE);
                }
            }
        }
    }

    fonsClearState(m_fontContext->m_fsContext);

    if (glfonsVertices(m_fontContext->m_fsContext, &vertData, &nVerts)) {
        _mesh.addVertices((GLbyte*)vertData.data(), nVerts);
    }
}

void FontStyle::buildPolygon(Polygon& _polygon, std::string& _layer, Properties& _props, VboMesh& _mesh) {

}

void FontStyle::prepareDataProcessing(MapTile& _tile) {
    m_fontContext->m_contextMutex->lock();

    m_processedTile = &_tile;

    fsuint buffer;
    
    int transformTextureSize = 2; // start with a transform texture of size 2x4 

    glfonsBufferCreate(m_fontContext->m_fsContext, transformTextureSize, &buffer);
    _tile.setTextBuffer(*this, buffer);
    glfonsBindBuffer(m_fontContext->m_fsContext, buffer);
}

void FontStyle::finishDataProcessing(MapTile& _tile) {
    glfonsBindBuffer(m_fontContext->m_fsContext, 0);

    m_processedTile = nullptr;

    m_fontContext->m_contextMutex->unlock();
}

void FontStyle::setupForTile(const MapTile& _tile) {
    std::unique_ptr<Texture>& texture = m_transformTileTextures[_tile.getID()];

    texture->bind();
    
    // transform texture
    m_shaderProgram->setUniformi("u_transforms", texture->getTextureSlot()); 
    // resolution of the transform texture
    m_shaderProgram->setUniformf("u_tresolution", texture->getWidth(), texture->getHeight()); 
}

void FontStyle::setup(View& _view) {
    float projectionMatrix[16] = {0};

    glfonsProjection(m_fontContext->m_fsContext, projectionMatrix);

    for (auto& pair : m_transformTileTextures) {
        pair.second->update();
    }

    m_atlas->update();
    m_atlas->bind();

    m_shaderProgram->setUniformi("u_tex", m_atlas->getTextureSlot()); // atlas
    m_shaderProgram->setUniformf("u_resolution", _view.getWidth(), _view.getHeight());
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

void createTexTransforms(void* _userPtr, unsigned int _width, unsigned int _height) {
    FontStyle* fontStyle = static_cast<FontStyle*>(_userPtr);

    TextureOptions options = {GL_RGBA, GL_RGBA, {GL_LINEAR, GL_LINEAR}, {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE}};

    std::unique_ptr<Texture> texture(new Texture(_width, _height, 1 /* gpu slot */ , options));

    fontStyle->m_transformTileTextures[fontStyle->m_processedTile->getID()] = std::move(texture);
}

void updateTransforms(void* _userPtr, unsigned int _xoff, unsigned int _yoff, unsigned int _width,
                      unsigned int _height, const unsigned int* _pixels, void* _ownerPtr) {

    FontStyle* fontStyle = static_cast<FontStyle*>(_userPtr);
    MapTile* tile = static_cast<MapTile*>(_ownerPtr);

    const GLuint* subData = static_cast<const GLuint*>(_pixels);

    fontStyle->m_transformTileTextures[tile->getID()]->setSubData(subData, _xoff, _yoff, _width, _height);
}

void updateAtlas(void* _userPtr, unsigned int _xoff, unsigned int _yoff,
                 unsigned int _width, unsigned int _height, const unsigned int* _pixels) {

    FontStyle* fontStyle = static_cast<FontStyle*>(_userPtr);
    fontStyle->m_atlas->setSubData(static_cast<const GLuint*>(_pixels), _xoff, _yoff, _width, _height);
}

void createAtlas(void* _userPtr, unsigned int _width, unsigned int _height) {

    FontStyle* fontStyle = static_cast<FontStyle*>(_userPtr);
    fontStyle->m_atlas = std::unique_ptr<Texture>(new Texture(_width, _height));
}

bool errorCallback(void* _userPtr, fsuint buffer, GLFONSError error) {
    FontStyle* fontStyle = static_cast<FontStyle*>(_userPtr);

    bool solved = false;

    switch (error) {
        case GLFONSError::ID_OVERFLOW: {

            logMsg("[FontStyle] FontError : ID_OVERFLOW in text buffer %d\n", buffer);

            std::unique_ptr<Texture>& texture = fontStyle->m_transformTileTextures[fontStyle->m_processedTile->getID()];

            // expand the transform texture in cpu side
            glfonsExpandTransform(fontStyle->m_fontContext->m_fsContext, buffer, texture->getWidth() * 2);

            // double size of texture
            texture->resize(texture->getWidth() * 2, texture->getHeight() * 2);

            solved = true; // error solved
            
            break;
        }

        default:
            logMsg("[FontStyle] FontError : undefined error\n");
            break;
    }

    return solved;
}

void FontStyle::initFontContext(const std::string& _fontFile) {

    int atlasWidth = 512;
    int atlasHeight = 512;

    GLFONSparams params;

    params.errorCallback = errorCallback;
    params.createAtlas = createAtlas;
    params.createTexTransforms = createTexTransforms;
    params.updateAtlas = updateAtlas;
    params.updateTransforms = updateTransforms;

    FONScontext* context = glfonsCreate(atlasWidth, atlasHeight, FONS_ZERO_TOPLEFT, params, (void*) this);

    unsigned int dataSize;
    unsigned char* data = bytesFromResource(_fontFile.c_str(), &dataSize);
    m_font = fonsAddFont(context, "droid-serif", data, dataSize);

    if (m_font == FONS_INVALID) {
        logMsg("[FontStyle] Error loading font file %s\n", _fontFile.c_str());
    }

    m_fontContext = std::shared_ptr<FontContext>(new FontContext {
        std_patch::make_unique<std::mutex>(), context
    });

}
