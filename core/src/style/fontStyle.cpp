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

    if(_layer.compare("roads") == 0) {
        for(auto prop : _props.stringProps) {
            if(prop.first.compare("name") == 0) {
                fsuint textId;

                glfonsGenText(m_fontContext, 1, &textId);
                glfonsRasterize(m_fontContext, textId, prop.second.c_str(), FONS_EFFECT_NONE);
            }
        }
    }

    std::vector<float> vertData;

    if(glfonsVertices(m_fontContext, &vertData)) {
        _mesh.addVertices(reinterpret_cast<GLbyte*>(&vertData[0]), vertData.size());
    }
    
}

void FontStyle::buildPolygon(Polygon& _polygon, std::string& _layer, Properties& _props, VboMesh& _mesh) {

}

void FontStyle::prepareDataProcessing(MapTile& _tile) {
    m_buildMutex.lock();

    fsuint buffer;
    
    glfonsBufferCreate(m_fontContext, 32, &buffer);
    m_tileBuffers[_tile.getID()] = buffer;
    glfonsBindBuffer(m_fontContext, buffer);

}

void FontStyle::finishDataProcessing(MapTile& _tile) {
    glfonsBindBuffer(m_fontContext, 0);

    m_buildMutex.unlock();
}

void FontStyle::setup() {
}

void createTexTransforms(void* _userPtr, unsigned int _width, unsigned int _height) {
    logMsg("create tex transforms\n");
}

void updateTransforms(void* _userPtr, unsigned int _xoff, unsigned int _yoff,
                      unsigned int _width, unsigned int _height, const unsigned int* _pixels) {
    logMsg("update transforms\n");
}

void updateAtlas(void* _userPtr, unsigned int _xoff, unsigned int _yoff,
                 unsigned int _width, unsigned int _height, const unsigned int* _pixels) {
    logMsg("update atlas\n");
}

void createAtlas(void* _usrPtr, unsigned int _width, unsigned int _height) {
    logMsg("create atlas\n");
}

void FontStyle::initFontContext(const std::string& _fontFile) {
    GLFONSparams params;

    params.createAtlas = createAtlas;
    params.createTexTransforms = createTexTransforms;
    params.updateAtlas = updateAtlas;
    params.updateTransforms = updateTransforms;

    m_fontContext = glfonsCreate(512, 512, FONS_ZERO_TOPLEFT, params, (void*) this);

    unsigned int dataSize;
    unsigned char* data = bytesFromResource(_fontFile.c_str(), &dataSize);
    m_font = fonsAddFont(m_fontContext, "droid-serif", data, dataSize);

    if(m_font == FONS_INVALID) {
        logMsg("Error loading font file %s\n", _fontFile.c_str());
    }

}
