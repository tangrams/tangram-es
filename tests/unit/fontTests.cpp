
int main(int argc, char *argv[]) {
    
    return 0;
}

#if 0
#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"

#include <iostream>
#include <vector>
#include "tangram.h"
#include "platform.h"
#include "gl.h"
#define FONTSTASH_IMPLEMENTATION
#include "fontstash.h"

#define ATLAS_WIDTH  512
#define ATLAS_HEIGHT 512

struct UserPtr {
    int screenRes;
    fsuint bufferId;
};

void updateAtlas(void* _userPtr, unsigned int _xoff, unsigned int _yoff,
                         unsigned int _width, unsigned int _height, const unsigned int* _pixels) {}

void createAtlas(void* _userPtr, unsigned int _width, unsigned int _height) {
    REQUIRE(_width == ATLAS_WIDTH);
    REQUIRE(_height == ATLAS_HEIGHT);
}

FONScontext* initContext(void* _usrPtr) {
    GLFONSparams params;

    params.useGLBackend = false;
    params.updateAtlas = updateAtlas;
    params.updateBuffer = nullptr;

    return glfonsCreate(ATLAS_WIDTH, ATLAS_HEIGHT, FONS_ZERO_TOPLEFT, params, _usrPtr);
}

int initFont(FONScontext* _context) {
    unsigned int dataSize;
    unsigned char* data = bytesFromFile("fonts/Roboto-Regular.ttf", PathType::internal, &dataSize);

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

TEST_CASE( "Test that the number of vertices correspond to the logic", "[Core][Fontstash][glfonsVertices]" ) {
    UserPtr p;

    FONScontext* context = initContext(&p);
    REQUIRE(context != NULL);

    int font = initFont(context);
    REQUIRE(font >= 0);

    glfonsBufferCreate(context, &p.bufferId);
    glfonsBindBuffer(context, p.bufferId);

    fonsSetSize(context, 15.0);
    fonsSetFont(context, font);

    fsuint id;
    glfonsGenText(context, 1, &id);

    std::string text("tangram");
    glfonsRasterize(context, id, text.c_str());

    std::vector<float> vertices;
    int size = glfonsVerticesSize(context);
    // 6 vertices per glyph - 4 float attributes per vertex
    vertices.resize(size * 6 * 4);
    glfonsVertices(context, static_cast<float*>(vertices.data()));

    REQUIRE(size == text.size() * 6); // shoud have 6 vertices per glyph

    glfonsDelete(context);
}
#endif
