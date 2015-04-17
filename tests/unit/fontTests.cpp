#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"

#include <iostream>
#include <vector>
#include "tangram.h"
#define GLFONTSTASH_IMPLEMENTATION
#include "glfontstash.h"

#define ATLAS_WIDTH         512
#define ATLAS_HEIGHT        512
#define TEXT_BUFFER_SIZE    32
#define ID_OVERFLOW_SIZE    TEXT_BUFFER_SIZE * TEXT_BUFFER_SIZE

struct Text {
    fsuint textId;
    float x;
    float y;
    float rotation;
    float alpha;
};

struct UserPtr {
    int screenRes;
    fsuint bufferId;
    std::vector<Text> texts;
};

void createTexTransforms(void* _userPtr, unsigned int _width, unsigned int _height) {
    REQUIRE(_width == TEXT_BUFFER_SIZE);
    REQUIRE(_height == TEXT_BUFFER_SIZE * 2);
}

void updateTransforms(void* _userPtr, unsigned int _xoff, unsigned int _yoff, unsigned int _width,
                              unsigned int _height, const unsigned int* _pixels, void* _ownerPtr) {
    UserPtr* ptr = static_cast<UserPtr*>(_userPtr);

    float screenRes = (float) ptr->screenRes;

    for(auto text : ptr->texts) {
        float i = fmod(text.textId * 2, TEXT_BUFFER_SIZE);
        float j = (text.textId * 2) / TEXT_BUFFER_SIZE;

        REQUIRE(i < TEXT_BUFFER_SIZE);
        REQUIRE(j < TEXT_BUFFER_SIZE * 2);

        unsigned int p = (unsigned int) (j * TEXT_BUFFER_SIZE + i);

        unsigned int packByte1 = _pixels[p];
        unsigned int packByte2 = _pixels[p + 1];

        int x = ((packByte1 & 0x000000ff) >>  0);
        int y = ((packByte1 & 0x0000ff00) >>  8);
        int r = ((packByte1 & 0x00ff0000) >> 16);
        int a = ((packByte1 & 0xff000000) >> 24);

        REQUIRE(a < 256); REQUIRE(a >= 0);
        REQUIRE(r < 256); REQUIRE(r >= 0);
        REQUIRE(x < 256); REQUIRE(x >= 0);
        REQUIRE(y < 256); REQUIRE(y >= 0);

        int px = (packByte2 & 0x000000ff) >> 0;
        int py = (packByte2 & 0x0000ff00) >> 8;

        REQUIRE(px < 256); REQUIRE(px >= 0);
        REQUIRE(py < 256); REQUIRE(py >= 0);

        float txe = screenRes / 255.0; // max translation x error
        float tye = screenRes / 255.0; // max translation y error

        float rx = (x / 255.0) * screenRes;
        float ry = (y / 255.0) * screenRes;

        // tests that the original results fits inside results with its logical error
        REQUIRE(text.x < rx + txe);
        REQUIRE(text.y < ry + tye);
        REQUIRE(text.x > rx - txe);
        REQUIRE(text.y > ry - tye);

        // compute the result and add the precision, see text.vs
        rx = rx + txe * (px / 255.0);
        ry = ry + tye * (py / 255.0);

        // result error
        float dx = fabs(text.x - rx);
        float dy = fabs(text.y - ry);

        // theoritical floating point precision error on precision encoding, see glfontstash.h
        float perror = 0.00196f;

        float epxe = txe * perror; // theoritical encoded error on translation x
        float epye = tye * perror; // theoritical encoded error on translation y

        // test that final result with its precision added is lower than the theoritical one
        REQUIRE(dx <= epxe);
        REQUIRE(dy <= epye);
    }
}

void updateAtlas(void* _userPtr, unsigned int _xoff, unsigned int _yoff,
                         unsigned int _width, unsigned int _height, const unsigned int* _pixels) {

}

void createAtlas(void* _userPtr, unsigned int _width, unsigned int _height) {
    REQUIRE(_width == ATLAS_WIDTH);
    REQUIRE(_height == ATLAS_HEIGHT);
}

bool errorCallback(void* _userPtr, fsuint buffer, GLFONSError error) {
    UserPtr* ptr = static_cast<UserPtr*>(_userPtr);

    REQUIRE(error == GLFONSError::ID_OVERFLOW);
    REQUIRE(ptr->bufferId == buffer);

    return false;
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
    UserPtr p;

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
    UserPtr p;

    FONScontext* context = initContext(&p);
    int font = initFont(context);

    glfonsBufferCreate(context, TEXT_BUFFER_SIZE, &p.bufferId);
    glfonsBindBuffer(context, p.bufferId);

    fonsSetSize(context, 15.0);
    fonsSetFont(context, font);

    fsuint id;
    glfonsGenText(context, 1, &id);

    std::string text("tangram");
    glfonsRasterize(context, id, text.c_str());

    std::vector<float> vertices;
    int size = glfonsVerticesSize(context);
    vertices.resize(size * INNER_DATA_OFFSET);
    glfonsVertices(context, reinterpret_cast<float*>(vertices.data()));

    REQUIRE(size == text.size() * 6); // shoud have 6 vertices per glyph

    glfonsDelete(context);
}

TEST_CASE( "Test that unpacking the encoded transforms give expected results", "[Core][Fontstash][glfonsTransform]" ) {
    UserPtr p;
    FONScontext* context = initContext(&p);
    int font = initFont(context);

    glfonsBufferCreate(context, TEXT_BUFFER_SIZE, &p.bufferId);
    glfonsBindBuffer(context, p.bufferId);

    fonsSetSize(context, 15.0);
    fonsSetFont(context, font);

    p.screenRes = 1024;

    glfonsScreenSize(context, p.screenRes, p.screenRes);

    for(int i = 0; i < 1024; i+=24) {
        Text text;

        glfonsGenText(context, 1, &text.textId);

        text.x = i;
        text.y = i / 2.0;
        text.rotation = i * M_PI;
        text.alpha = 0.5;

        std::string str("tangram");
        glfonsRasterize(context, text.textId, str.c_str());
        glfonsTransform(context, text.textId, text.x, text.y, text.rotation, text.alpha);

        p.texts.push_back(text);
    }

    glfonsUpdateTransforms(context, nullptr);

    std::vector<float> vertices;
    int size = glfonsVerticesSize(context);
    vertices.resize(size * INNER_DATA_OFFSET);
    glfonsVertices(context, reinterpret_cast<float*>(vertices.data()));

    glfonsDelete(context);
}
