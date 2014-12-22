#include "fontStyle.h"
#define GLFONTSTASH_IMPLEMENTATION
#include "fontstash/glfontstash.h"

FontStyle::FontStyle(const std::string& _fontFile, std::string _name, GLenum _drawMode) : Style(_name, _drawMode) {
    constructVertexLayout();
    constructShaderProgram();
    initFontContext();
}

FontStyle::~FontStyle() {
    glfonsDelete(fontContext);
}

void FontStyle::constructVertexLayout() {
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
    }));
}

void FontStyle::constructShaderProgram() {
    std::string vertShaderSrcStr = stringFromResource("text.vs");

    std::string fragShaderSrcStr = stringFromResource("text.fs");

    m_shaderProgram = std::make_shared<ShaderProgram>();
    m_shaderProgram->buildFromSourceStrings(fragShaderSrcStr, vertShaderSrcStr);
}

void FontStyle::buildPoint(Point& _point, std::string& _layer, Properties& _props, VboMesh& _mesh) {

}

void FontStyle::buildLine(Line& _line, std::string& _layer, Properties& _props, VboMesh& _mesh) {

}

void FontStyle::buildPolygon(Polygon& _polygon, std::string& _layer, Properties& _props, VboMesh& _mesh) {

}

void FontStyle::prepareDataProcessing(MapTile &_tile) {
    fsuint buffer;
    glfonsBufferCreate(fontContext, 32, &buffer);
    tileBuffers[_tile.getID()] = buffer;
    glfonsBindBuffer(fontContext, buffer);
}

void FontStyle::finishDataProcessing(MapTile &_tile) {
    glfonsBindBuffer(fontContext, 0);
}

void FontStyle::setup() {
}

void errorCallback(void* _userPtr, GLFONSbuffer* _buffer, GLFONSError _fonsError) {
    logMsg("glfontstash error\n");
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

void vertexData(void* _userPtr, unsigned int _nVerts, const float* _data) {
    logMsg("vertex data\n");
}

void createAtlas(void* usrPtr, unsigned int width, unsigned int height) {
    logMsg("create atlas\n");
}

void FontStyle::initFontContext() {
    GLFONSparams params;

    params.errorCallback = errorCallback;
    params.createAtlas = createAtlas;
    params.createTexTransforms = createTexTransforms;
    params.updateAtlas = updateAtlas;
    params.updateTransforms = updateTransforms;
    params.vertexData = vertexData;

    fontContext = glfonsCreate(512, 512, FONS_ZERO_TOPLEFT, params, (void*) this);
}
