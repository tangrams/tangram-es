#include "fontStyle.h"
#define GLFONTSTASH_IMPLEMENTATION
#include "fontstash/glfontstash.h"
#include "glm/gtc/matrix_transform.hpp"

FontStyle::FontStyle(const std::string& _fontFile, std::string _name, float _fontSize, GLenum _drawMode)
: Style(_name, _drawMode), m_fontSize(_fontSize) {

    constructVertexLayout();
    constructShaderProgram();
    initFontContext(_fontFile);
}

FontStyle::~FontStyle() {
    glfonsDelete(m_fontContext->m_fsContext);
    for (auto pair : m_tileTexTransforms) {
        glDeleteTextures(1, &pair.second);
    }
    glDeleteTextures(1, &m_atlas);
}

void FontStyle::constructVertexLayout() {
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 2, GL_FLOAT, false, 0},
        {"a_texCoord", 2, GL_FLOAT, false, 0},
        {"a_fsid", 1, GL_FLOAT, false, 0},
    }));
}

void FontStyle::constructShaderProgram() {
    std::string vertShaderSrcStr = stringFromResource("text.vs");
    std::string fragShaderSrcStr = stringFromResource("text.fs");

    m_shaderProgram = std::make_shared<ShaderProgram>();

    if (!m_shaderProgram->buildFromSourceStrings(fragShaderSrcStr, vertShaderSrcStr)) {
        logMsg("Error building text shader program\n");
    }
}

void FontStyle::buildPoint(Point& _point, std::string& _layer, Properties& _props, VboMesh& _mesh) {

}

void FontStyle::buildLine(Line& _line, std::string& _layer, Properties& _props, VboMesh& _mesh) {
    std::vector<float> vertData;
    int nVerts = 0;

    fonsSetSize(m_fontContext->m_fsContext, m_fontSize);
    fonsSetFont(m_fontContext->m_fsContext, m_font);

    if (_layer.compare("roads") == 0) {
        for (auto prop : _props.stringProps) {
            if (prop.first.compare("name") == 0) {
                fsuint textId;

                glfonsGenText(m_fontContext->m_fsContext, 1, &textId);
                glfonsRasterize(m_fontContext->m_fsContext, textId, prop.second.c_str(), FONS_EFFECT_NONE);

                glm::dvec2 p1 = glm::dvec2(_line[0]);
                glm::dvec2 p2 = glm::dvec2(_line[1]);

                glm::dvec2 p1p2 = glm::normalize(p1 - p2);
                double r = atan2(p1p2.x, p1p2.y) + M_PI_2;

                glm::dvec2 middle = (p1 + p2) / 2.0;

                std::unique_ptr<Label> label(new Label {
                    m_fontContext,
                    textId,
                    prop.second.c_str(),
                    middle,     // world position
                    1.0,        // alpha
                    (float)r    // rotation
                });

                m_processedTile->addLabel(*this, std::move(label));
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
    
    glfonsBufferCreate(m_fontContext->m_fsContext, 32, &buffer);
    _tile.setTextBuffer(*this, buffer);
    glfonsBindBuffer(m_fontContext->m_fsContext, buffer);
}

void FontStyle::finishDataProcessing(MapTile& _tile) {
    glfonsBindBuffer(m_fontContext->m_fsContext, 0);

    m_processedTile = nullptr;

    m_fontContext->m_contextMutex->unlock();
}

GLuint FontStyle::textureTransformName(const TileID _tileId) const {

    auto it = m_tileTexTransforms.find(_tileId);

    if (it != m_tileTexTransforms.end()) {
        return it->second;
    }

    return 0; // non-valid texture name
}

void FontStyle::setupForTile(const MapTile& _tile) {

    GLuint textureName = textureTransformName(_tile.getID());

    if(textureName != 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureName);

        m_shaderProgram->setUniformi("u_transforms", 1); // transform texture
    }
}

void FontStyle::processTileTransformCreation() {

    while (m_pendingTileTexTransforms.size() > 0) {
        logMsg("Create a texture transforms\n");

        std::pair<TileID, glm::vec2> pair = m_pendingTileTexTransforms.front();

        glm::vec2 size = pair.second;

        auto defaultTransforms = new unsigned int[(int)(size.x * size.y)] {0}; // zero filled

        GLuint texTransform;
        glGenTextures(1, &texTransform);
        glBindTexture(GL_TEXTURE_2D, texTransform);

        // fill texture with 0
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, defaultTransforms);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        delete[] defaultTransforms;

        m_tileTexTransforms[pair.first] = texTransform;

        m_pendingTileTexTransforms.pop();
    }

}

void FontStyle::processTileTransformUpdate() {

    while (m_pendingTexTransformsData.size() > 0) {
        TileTransform transformData = m_pendingTexTransformsData.front();

        logMsg("Update the texture transforms for tile [%d, %d, %d], %dx%d pixels\n", transformData.m_id.x,
               transformData.m_id.y, transformData.m_id.z, transformData.m_width, transformData.m_height);

        glBindTexture(GL_TEXTURE_2D, m_tileTexTransforms[transformData.m_id]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, transformData.m_xoff, transformData.m_yoff,
            transformData.m_width, transformData.m_height, GL_RGBA, GL_UNSIGNED_BYTE, transformData.m_pixels);

        glBindTexture(GL_TEXTURE_2D, 0);

        m_pendingTexTransformsData.pop();
    }

}

void FontStyle::processAtlasUpdate() {

    glBindTexture(GL_TEXTURE_2D, m_atlas);
    while (m_pendingTexAtlasData.size() > 0) {
        Atlas atlasData = m_pendingTexAtlasData.front();

        logMsg("Update the atlas, %dx%d pixels\n", atlasData.m_width, atlasData.m_height);

        glTexSubImage2D(GL_TEXTURE_2D, 0, atlasData.m_xoff, atlasData.m_yoff,
            atlasData.m_width, atlasData.m_height, GL_ALPHA, GL_UNSIGNED_BYTE, atlasData.m_pixels);

        m_pendingTexAtlasData.pop();
    }
    glBindTexture(GL_TEXTURE_2D, 0);

}

void FontStyle::setup(View& _view) {
    float projectionMatrix[16] = {0};

    // process pending opengl texture updates / creation
    processAtlasUpdate();
    processTileTransformCreation();
    processTileTransformUpdate();

    glfonsScreenSize(m_fontContext->m_fsContext, _view.getWidth(), _view.getHeight());
    glfonsProjection(m_fontContext->m_fsContext, projectionMatrix);

    // activate the atlas on the texture slot0, the texture transform is on slot1
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_atlas);

    m_shaderProgram->setUniformi("u_tex", 0); // atlas
    m_shaderProgram->setUniformf("u_tresolution", 32, 64); // resolution of transform texture
    m_shaderProgram->setUniformf("u_resolution", _view.getWidth(), _view.getHeight());
    m_shaderProgram->setUniformf("u_color", 1.0, 1.0, 1.0);
    m_shaderProgram->setUniformMatrix4f("u_proj", projectionMatrix);
}

void createTexTransforms(void* _userPtr, unsigned int _width, unsigned int _height) {
    FontStyle* fontStyle = static_cast<FontStyle*>(_userPtr);

    glm::vec2 size = glm::vec2(_width, _height);

    fontStyle->m_pendingTileTexTransforms.push(std::pair<TileID, glm::vec2>(fontStyle->m_processedTile->getID(), size));
}

void updateTransforms(void* _userPtr, unsigned int _xoff, unsigned int _yoff, unsigned int _width,
                      unsigned int _height, const unsigned int* _pixels, void* _ownerPtr) {
    FontStyle* fontStyle = static_cast<FontStyle*>(_userPtr);
    MapTile* tile = static_cast<MapTile*>(_ownerPtr);

    TileTransform texData(tile->getID());

    texData.m_pixels = _pixels;
    texData.m_xoff = _xoff;
    texData.m_yoff = _yoff;
    texData.m_width = _width;
    texData.m_height = _height;

    fontStyle->m_pendingTexTransformsData.push(texData);
}

void updateAtlas(void* _userPtr, unsigned int _xoff, unsigned int _yoff,
                 unsigned int _width, unsigned int _height, const unsigned int* _pixels) {
    FontStyle* fontStyle = static_cast<FontStyle*>(_userPtr);

    Atlas texData;

    texData.m_pixels = _pixels;
    texData.m_height =_height;
    texData.m_width = _width;
    texData.m_xoff = _xoff;
    texData.m_yoff = _yoff;

    fontStyle->m_pendingTexAtlasData.push(texData);
}

void createAtlas(void* _userPtr, unsigned int _width, unsigned int _height) {
    logMsg("create atlas");
    FontStyle* fontStyle = static_cast<FontStyle*>(_userPtr);

    glGenTextures(1, &fontStyle->m_atlas);
    glBindTexture(GL_TEXTURE_2D, fontStyle->m_atlas);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, _width, _height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void FontStyle::initFontContext(const std::string& _fontFile) {

    int atlasWidth = 512;
    int atlasHeight = 512;

    GLFONSparams params;

    params.createAtlas = createAtlas;
    params.createTexTransforms = createTexTransforms;
    params.updateAtlas = updateAtlas;
    params.updateTransforms = updateTransforms;

    FONScontext* context = glfonsCreate(atlasWidth, atlasHeight, FONS_ZERO_TOPLEFT, params, (void*) this);

    unsigned int dataSize;
    unsigned char* data = bytesFromResource(_fontFile.c_str(), &dataSize);
    m_font = fonsAddFont(context, "droid-serif", data, dataSize);

    if (m_font == FONS_INVALID) {
        logMsg("Error loading font file %s\n", _fontFile.c_str());
    }

    m_fontContext = std::shared_ptr<FontContext>(new FontContext {
        std_patch::make_unique<std::mutex>(),
        context
    });

}
