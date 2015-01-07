#define CATCH_CONFIG_MAIN 
#include "catch/catch.hpp"

#include <iostream>
#include <vector>
#include "tangram.h"
#define GLFONTSTASH_IMPLEMENTATION
#include "fontstash/glfontstash.h"

#define ATLAS_WIDTH         512
#define ATLAS_HEIGHT        512
#define TEXT_BUFFER_SIZE    32
#define ID_OVERFLOW_SIZE    TEXT_BUFFER_SIZE * TEXT_BUFFER_SIZE

struct userPtr {
    fsuint bufferId;
};

void createTexTransforms(void* _userPtr, unsigned int _width, unsigned int _height) {
    REQUIRE(_width == TEXT_BUFFER_SIZE);
    REQUIRE(_height == TEXT_BUFFER_SIZE * 2);
}

void updateTransforms(void* _userPtr, unsigned int _xoff, unsigned int _yoff, unsigned int _width,
                              unsigned int _height, const unsigned int* _pixels, void* _ownerPtr) {

}

void updateAtlas(void* _userPtr, unsigned int _xoff, unsigned int _yoff,
                         unsigned int _width, unsigned int _height, const unsigned int* _pixels) {

}

void createAtlas(void* _userPtr, unsigned int _width, unsigned int _height) {
    REQUIRE(_width == ATLAS_WIDTH);
    REQUIRE(_height == ATLAS_HEIGHT);
}

void errorCallback(void* _userPtr, fsuint buffer, GLFONSError error) {
    userPtr* ptr = static_cast<userPtr*>(_userPtr);

    REQUIRE(error == GLFONSError::ID_OVERFLOW);
    REQUIRE(ptr->bufferId == buffer);
}

FONScontext* initContext(void* _usrPtr) {
    GLFONSparams params;

    params.errorCallback = errorCallback;
    params.createAtlas = createAtlas;
    params.createTexTransforms = createTexTransforms;
    params.updateAtlas = updateAtlas;
    params.updateTransforms = updateTransforms;

    return glfonsCreate(ATLAS_WIDTH, ATLAS_HEIGHT, FONS_ZERO_TOPLEFT, params, _usrPtr);
}

int initFont(FONScontext* _context) {
    unsigned int dataSize;
    unsigned char* data = bytesFromResource("Roboto-Regular.ttf", &dataSize);

    return fonsAddFont(_context, "droid-serif", data, dataSize);
}

TEST_CASE( "Test context initialization", "[Core][Fontstash][glfonsCreate]" ) {
    FONScontext* context = initContext(nullptr);

    REQUIRE(context != NULL);

    glfonsDelete(context);
}

TEST_CASE( "Test .ttf file initialization", "[Core][Fontstash][fonsAddFont]" ) {
    FONScontext* context = initContext(nullptr);
    int font = initFont(context);

    REQUIRE(font != FONS_INVALID);

    glfonsDelete(context);
}

TEST_CASE( "Test the buffer creation and the size of the required texture transform", "[Core][Fontstash][glfonsBufferCreate]" ) {
    FONScontext* context = initContext(nullptr);

    fsuint buffer;
    glfonsBufferCreate(context, TEXT_BUFFER_SIZE, &buffer);

    glfonsDelete(context);
}

TEST_CASE( "Test that the overflow callback gets called for the right overflow size", "[Core][Fontstash][errorCallback]" ) {
    userPtr p;

    FONScontext* context = initContext(&p);
    int font = initFont(context);

    glfonsBufferCreate(context, TEXT_BUFFER_SIZE, &p.bufferId);
    glfonsBindBuffer(context, p.bufferId);

    fsuint id[ID_OVERFLOW_SIZE + 1];
    // create an overflow
    glfonsGenText(context, ID_OVERFLOW_SIZE + 1, id);

    glfonsDelete(context);
}

TEST_CASE( "Test that the number of vertices correspond to the logic", "[Core][Fontstash][glfonsVertices]" ) {
    userPtr p;  

    FONScontext* context = initContext(&p);
    int font = initFont(context);

    glfonsBufferCreate(context, TEXT_BUFFER_SIZE, &p.bufferId);
    glfonsBindBuffer(context, p.bufferId);

    fonsSetSize(context, 15.0);
    fonsSetFont(context, font);

    fsuint id;
    glfonsGenText(context, 1, &id);

    std::string text("tangram");
    glfonsRasterize(context, id, text.c_str(), FONS_EFFECT_NONE);

    std::vector<float> vertices;
    int nverts = 0;
    glfonsVertices(context, &vertices, &nverts);

    REQUIRE(nverts == text.size() * 6); // shoud have 6 vertices per glyph

    glfonsDelete(context);
}


