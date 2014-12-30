#include "fontStyle.h"
#define GLFONTSTASH_IMPLEMENTATION
#include "fontstash/glfontstash.h"

FontStyle::FontStyle(const std::string& _fontFile, std::string _name, GLenum _drawMode) : Style(_name, _drawMode) {
    constructVertexLayout();
    constructShaderProgram();
    initFontContext(_fontFile);
}

FontStyle::~FontStyle() {
    glfonsDelete(m_fontContext);

    for(auto pair : m_tileTexTransforms) {
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

    if(!m_shaderProgram->buildFromSourceStrings(fragShaderSrcStr, vertShaderSrcStr)) {
        logMsg("Error building text shader program\n");
    }
}

void FontStyle::buildPoint(Point& _point, std::string& _layer, Properties& _props, VboMesh& _mesh) {

}

void FontStyle::buildLine(Line& _line, std::string& _layer, Properties& _props, VboMesh& _mesh) {
    std::vector<float> vertData;
    int nVerts = 0;

    fonsSetSize(m_fontContext, 40.0);
    fonsSetFont(m_fontContext, m_font);

    if(_layer.compare("roads") == 0) {
        for(auto prop : _props.stringProps) {
            if(prop.first.compare("name") == 0) {
                fsuint textId;

                glfonsGenText(m_fontContext, 1, &textId);
                glfonsRasterize(m_fontContext, textId, prop.second.c_str(), FONS_EFFECT_NONE);

                m_tileLabels[m_processedTile->getID()].push_back(textId);

                static int y = 0;
                y++;
                glfonsTransform(m_fontContext, textId, 50.0, 200.0 + 15.0 * y, 0.0, 1.0);
            }
        }
    }

    glfonsUpdateTransforms(m_fontContext);

    fonsClearState(m_fontContext);

    if(glfonsVertices(m_fontContext, &vertData, &nVerts)) {
        _mesh.addVertices((GLbyte*)vertData.data(), nVerts);
    }
}

void FontStyle::buildPolygon(Polygon& _polygon, std::string& _layer, Properties& _props, VboMesh& _mesh) {

}

void FontStyle::prepareDataProcessing(MapTile& _tile) {
    m_buildMutex.lock();

    m_processedTile = &_tile;

    fsuint buffer;
    
    glfonsBufferCreate(m_fontContext, 32, &buffer);
    _tile.setTextBuffer(buffer);
    glfonsBindBuffer(m_fontContext, buffer);
}

void FontStyle::finishDataProcessing(MapTile& _tile) {
    glfonsBindBuffer(m_fontContext, 0);

    m_processedTile = nullptr;

    m_buildMutex.unlock();
}

void FontStyle::setup() {

    while(m_pendingTileTexTransforms.size() > 0) {
        logMsg("create tex transforms\n");
        std::pair<MapTile*, glm::vec2> pair = m_pendingTileTexTransforms.top();

        glm::vec2 size = pair.second;
        MapTile* tile = pair.first;

        GLuint texTransform;
        glGenTextures(1, &texTransform);
        glBindTexture(GL_TEXTURE_2D, texTransform);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        tile->setTexTransform(texTransform);
        
        m_tileTexTransforms[tile->getID()] = texTransform;

        m_pendingTileTexTransforms.pop();
    }

    while(m_pendingTexTransformsData.size() > 0) {
        logMsg("update transforms\n");
        TileTransform transformData = m_pendingTexTransformsData.top();

        glBindTexture(GL_TEXTURE_2D, m_tileTexTransforms[transformData.m_id]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, transformData.m_xoff, transformData.m_yoff,
                        transformData.m_width, transformData.m_height, GL_RGBA, GL_UNSIGNED_BYTE, transformData.m_pixels);

        glBindTexture(GL_TEXTURE_2D, 0);

        m_pendingTexTransformsData.pop();
    }

    glBindTexture(GL_TEXTURE_2D, m_atlas);
    while(m_pendingTexAtlasData.size() > 0) {
        logMsg("update atlas\n");
        Atlas atlasData = m_pendingTexAtlasData.top();

        glTexSubImage2D(GL_TEXTURE_2D, 0, atlasData.m_xoff, atlasData.m_yoff,
                        atlasData.m_width, atlasData.m_height, GL_ALPHA, GL_UNSIGNED_BYTE, atlasData.m_pixels);

        m_pendingTexAtlasData.pop();
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    // for now use compute projection matrix by hand
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    float projectionMatrix[16] = {0};
    // TODO : get those values from rendering context
    float width = (float) viewport[2];
    float height = (float) viewport[3];

    float r = width;
    float l = 0.0;
    float b = height;
    float t = 0.0;
    float n = -1.0;
    float f = 1.0;

    // could be simplified, exposing it like this for comprehension
    projectionMatrix[0] = 2.0 / (r-l);
    projectionMatrix[5] = 2.0 / (t-b);
    projectionMatrix[10] = 2.0 / (f-n);
    projectionMatrix[12] = -(r+l)/(r-l);
    projectionMatrix[13] = -(t+b)/(t-b);
    projectionMatrix[14] = -(f+n)/(f-n);
    projectionMatrix[15] = 1.0;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_atlas);

    m_shaderProgram->setUniformi("u_tex", 0); // atlas
    m_shaderProgram->setUniformf("u_tresolution", 32, 64); // resolution of transform texture
    m_shaderProgram->setUniformf("u_resolution", width, height);
    m_shaderProgram->setUniformf("u_color", 1.0, 1.0, 1.0);
    m_shaderProgram->setUniformMatrix4f("u_proj", projectionMatrix);
}

void createTexTransforms(void* _userPtr, unsigned int _width, unsigned int _height) {
    FontStyle* fontStyle = static_cast<FontStyle*>(_userPtr);

    glm::vec2 size = glm::vec2(_width, _height);

    fontStyle->m_pendingTileTexTransforms.push(std::pair<MapTile*, glm::vec2>(fontStyle->m_processedTile, size));
}

void updateTransforms(void* _userPtr, unsigned int _xoff, unsigned int _yoff,
                      unsigned int _width, unsigned int _height, const unsigned int* _pixels) {
    FontStyle* fontStyle = static_cast<FontStyle*>(_userPtr);

    TileTransform texData(fontStyle->m_processedTile->getID());

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
    GLFONSparams params;

    params.createAtlas = createAtlas;
    params.createTexTransforms = createTexTransforms;
    params.updateAtlas = updateAtlas;
    params.updateTransforms = updateTransforms;

    // TODO : get this from platform rendering context
    float screenWidth = 640;
    float screenHeight = 960;

    m_fontContext = glfonsCreate(512, 512, FONS_ZERO_TOPLEFT, params, screenWidth, screenHeight, (void*) this);

    unsigned int dataSize;
    unsigned char* data = bytesFromResource(_fontFile.c_str(), &dataSize);
    m_font = fonsAddFont(m_fontContext, "droid-serif", data, dataSize);

    if(m_font == FONS_INVALID) {
        logMsg("Error loading font file %s\n", _fontFile.c_str());
    }

}
