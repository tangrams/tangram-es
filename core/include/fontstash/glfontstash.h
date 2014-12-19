//
// Copyright (c) 2009-2013 Mikko Mononen memon@inside.org
// Copyright (c) 2014 Mapzen karim@mapzen.com
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
#ifndef GLFONTSTASH_H
#define GLFONTSTASH_H

#include <unordered_map>

#include "fontstash.h"

typedef unsigned int fsuint;

#define N_GLYPH_VERTS 6

typedef struct GLFONScontext GLFonscontext;

FONScontext* glfonsCreate(int width, int height, int flags);
void glfonsDelete(FONScontext* ctx);

void glfonsUploadTransforms(FONScontext* ctx);
void glfonsTransform(FONScontext* ctx, fsuint id, float tx, float ty, float r, float a);

void glfonsGenText(FONScontext* ctx, unsigned int nb, fsuint* textId);

void glfonsBufferCreate(FONScontext* ctx, unsigned int texTransformRes, fsuint* id);
void glfonsBufferDelete(FONScontext* gl, fsuint id);
void glfonsBindBuffer(FONScontext* ctx, fsuint id);

void glfonsRasterize(FONScontext* ctx, fsuint textId, const char* s, FONSeffectType effect);
void glfonsUploadVertices(FONScontext* ctx);

void glfonsGetBBox(FONScontext* ctx, fsuint id, float* x0, float* y0, float* x1, float* y1);
float glfonsGetGlyphOffset(FONScontext* ctx, fsuint id, int i);
float glfonsGetLength(FONScontext* ctx, fsuint id);
int glfonsGetGlyphCount(FONScontext* ctx, fsuint id);

unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

#endif

#ifdef GLFONTSTASH_IMPLEMENTATION

#define FONTSTASH_IMPLEMENTATION
#include "fontstash.h"

enum class GLFONSError {
    ID_OVERFLOW
};

struct GLFONSstash {
    int nbGlyph;
    float bbox[4];
    float length;
    float* glyphsXOffset;
};

struct GLFONSbuffer {
    fsuint id;
    fsuint idct;
    unsigned int* transformData;
    unsigned char* transformDirty;
    float* interleavedArray;
    unsigned int nbVerts;
    unsigned int maxId;
    int transformRes[2];
    std::unordered_map<fsuint, GLFONSstash*>* stashes;
};

struct GLFONSparams {
    void (*errorCallback)(void* usrPtr, GLFONSbuffer* buffer, GLFONSError error);
    void (*createTexTransforms)(void* usrPtr, unsigned int width, unsigned int height);
    void (*createAtlas)(void* usrPtr, unsigned int width, unsigned int height);
    void (*updateTransforms)(void* usrPtr, unsigned int xoff, unsigned int yoff,
                             unsigned int width, unsigned int height, const unsigned int* pixels);
    void (*updateAtlas)(void* usrPtr, unsigned int xoff, unsigned int yoff,
                        unsigned int width, unsigned int height, const unsigned int* pixels);
    void (*vertexData)(void* usrPtr, unsigned int nVerts, const float* data);
};

struct GLFONScontext {
    GLFONSparams params;
    int atlasRes[2];
    float screenSize[2];
    fsuint idct;
    fsuint boundBuffer;
    std::unordered_map<fsuint, GLFONSbuffer*>* buffers;
    void* userPtr;
};

void glfons__id2ij(GLFONScontext* gl, fsuint id, int* i, int* j) {
    int* res = gl->buffers->at(gl->boundBuffer)->transformRes;
    *i = (id*2) % (res[0]/2);
    *j = (id*2) / (res[0]/2);
}

static int glfons__renderCreate(void* userPtr, int width, int height) {
    GLFONScontext* gl = (GLFONScontext*)userPtr;

    gl->idct = 0;
    gl->buffers = new std::unordered_map<fsuint, GLFONSbuffer*>();
    gl->atlasRes[0] = width;
    gl->atlasRes[1] = height;
    gl->params.createAtlas(gl->userPtr, width, height);
    
    return 1;
}

static int glfons__renderResize(void* userPtr, int width, int height) {
    return 1;
}

static void glfons__renderUpdate(void* userPtr, int* rect, const unsigned char* data) {
    GLFONScontext* gl = (GLFONScontext*)userPtr;
    
    int h = rect[3] - rect[1];
    const unsigned char* subdata = data + rect[1] * gl->atlasRes[0];
    gl->params.updateAtlas(gl->userPtr, 0, rect[1], gl->atlasRes[0], h, reinterpret_cast<const unsigned int*>(subdata));
}

static void glfons__renderDraw(void* userPtr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts) {
    // called by fontstash, but has nothing to do
}

void glfons__freeStash(GLFONSstash* stash) {
    delete[] stash->glyphsXOffset;
    delete stash;
}

GLFONSbuffer* glfons__bufferBound(GLFONScontext* gl) {
    if(gl->boundBuffer == 0) {
        return nullptr;
    }

    return gl->buffers->at(gl->boundBuffer);
}

void glfonsUploadTransforms(FONScontext* ctx) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    GLFONSbuffer* buffer = glfons__bufferBound(gl);

    int inf = INT_MAX;
    int min = inf;
    int max = -inf;
    bool dirty = false;

    for(int i = 0; i < buffer->transformRes[1]; ++i) {
        if(buffer->transformDirty[i]) {
            dirty = true;
            if(min > max) {
                min = max = i;
            } else {
                max = i;
            }
        }
    }

    if(!dirty) {
        return;
    }

    // interleaved array stored as following in texture :
    // | x | y | rot | alpha | precision_x | precision_y | Ø | Ø |
    const unsigned int* subdata;
    subdata = buffer->transformData + min * buffer->transformRes[0];
    gl->params.updateTransforms(gl->userPtr, 0, min, buffer->transformRes[0], (max - min) + 1, subdata);
    std::fill(buffer->transformDirty, buffer->transformDirty + buffer->transformRes[1], 0);
}

void glfonsGenText(FONScontext* ctx, unsigned int nb, fsuint* textId) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    GLFONSbuffer* buffer = glfons__bufferBound(gl);

    if(buffer->idct + nb > buffer->maxId) {
        if(gl->params.errorCallback) {
            gl->params.errorCallback(gl->userPtr, buffer, GLFONSError::ID_OVERFLOW);
        }
        return;
    }

    for(int i = 0; i < nb; ++i) {
        textId[i] = buffer->idct++;
    }
}

void glfonsRasterize(FONScontext* ctx, fsuint textId, const char* s, FONSeffectType effect) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    GLFONSbuffer* buffer = glfons__bufferBound(gl);
    GLFONSstash* stash = new GLFONSstash;

    fonsDrawText(ctx, 0, 0, s, NULL, 0);

    float* data = nullptr;

    if(buffer->interleavedArray == nullptr) {
        buffer->interleavedArray = (float*) malloc(sizeof(float) * ctx->nverts * 5);
        buffer->nbVerts = 0;
        data = buffer->interleavedArray;
    } else {
        // TODO : realloc can be expensive, improve
        buffer->interleavedArray = (float*) realloc(buffer->interleavedArray,
                                                    sizeof(float) * (buffer->nbVerts + ctx->nverts) * 5);
        data = buffer->interleavedArray + buffer->nbVerts * 5;
    }

    stash->glyphsXOffset = new float[ctx->nverts / N_GLYPH_VERTS];

    int j = 0;
    for(int i = 0; i < ctx->nverts * 2; i += N_GLYPH_VERTS * 2) {
        stash->glyphsXOffset[j++] = ctx->verts[i];
    }

    float inf = std::numeric_limits<float>::infinity();
    float x0 = inf, x1 = -inf, y0 = inf, y1 = -inf;
    for(int i = 0, off = 0; i < ctx->nverts * 2; i += 2, off += 5) {
        GLfloat x, y, u, v;

        x = ctx->verts[i];
        y = ctx->verts[i+1];
        u = ctx->tcoords[i];
        v = ctx->tcoords[i+1];

        x0 = x < x0 ? x : x0;
        x1 = x > x1 ? x : x1;
        y0 = y < y0 ? y : y0;
        y1 = y > y1 ? y : y1;

        data[off] = x;
        data[off+1] = y;
        data[off+2] = u;
        data[off+3] = v;
        data[off+4] = float(textId);
    }

    stash->bbox[0] = x0; stash->bbox[1] = y0;
    stash->bbox[2] = x1; stash->bbox[3] = y1;

    stash->length = ctx->verts[(ctx->nverts*2)-2];

    if(ctx->shaping != NULL && fons__getState(ctx)->useShaping) {
        FONSshapingRes* res = ctx->shaping->shapingRes;
        stash->nbGlyph = res->glyphCount;
        fons__clearShaping(ctx);
    } else {
        stash->nbGlyph = (int)strlen(s);
    }

    buffer->nbVerts += ctx->nverts;

    // hack : reset fontstash state
    ctx->nverts = 0;

    buffer->stashes->insert(std::pair<fsuint, GLFONSstash*>(textId, stash));
}

void glfonsBufferCreate(FONScontext* ctx, unsigned int texTransformRes, fsuint* id) {
    if((texTransformRes & (texTransformRes-1)) != 0) {
        *id = 0;
        return;
    }

    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    GLFONSbuffer* buffer = new GLFONSbuffer;

    *id = ++gl->idct;

    buffer->idct = 0;
    buffer->transformRes[0] = texTransformRes;
    buffer->transformRes[1] = texTransformRes * 2;
    buffer->maxId = pow(texTransformRes, 2);
    buffer->transformData = new unsigned int[texTransformRes * texTransformRes * 2] ();
    buffer->transformDirty = new unsigned char[texTransformRes * 2] ();
    buffer->interleavedArray = nullptr;
    buffer->id = *id;
    buffer->stashes = new std::unordered_map<fsuint, GLFONSstash*>();

    gl->params.createTexTransforms(gl->userPtr, buffer->transformRes[0], buffer->transformRes[1]);

    gl->buffers->insert(std::pair<fsuint, GLFONSbuffer*>(*id, buffer));
}

void glfonsBufferDelete(GLFONScontext* gl, fsuint id) {
    GLFONSbuffer* buffer = gl->buffers->at(id);

    delete[] buffer->transformData;
    delete[] buffer->transformDirty;

    for(auto& elt : *buffer->stashes) {
        glfons__freeStash(elt.second);
    }
    buffer->stashes->clear();
    delete buffer->stashes;

    if(buffer->interleavedArray != nullptr) {
        free(buffer->interleavedArray);
    }

    gl->buffers->erase(buffer->id);
}

void glfonsBindBuffer(FONScontext* ctx, fsuint id) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    gl->boundBuffer = id;
}

static void glfons__renderDelete(void* userPtr) {
    GLFONScontext* gl = (GLFONScontext*)userPtr;

    for(auto& elt : *gl->buffers) {
        glfonsBufferDelete(gl, elt.first);
    }
    gl->buffers->clear();
    delete gl->buffers;
    delete gl;
}

FONScontext* glfonsCreate(int width, int height, int flags, GLFONSparams glParams, void* userPtr) {
    FONSparams params;
    GLFONScontext* gl = new GLFONScontext;
    gl->params = glParams;
    gl->userPtr = userPtr;

    params.width = width;
    params.height = height;
    params.flags = (unsigned char)flags;
    params.renderCreate = glfons__renderCreate;
    params.renderResize = glfons__renderResize;
    params.renderUpdate = glfons__renderUpdate;
    params.renderDraw = glfons__renderDraw;
    params.renderDelete = glfons__renderDelete;
    params.userPtr = gl;
    
    return fonsCreateInternal(&params);
}

void glfonsDelete(FONScontext* ctx) {
    fonsDeleteInternal(ctx);
}

unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    return (r) | (g << 8) | (b << 16) | (a << 24);
}

void glfonsUploadVertices(FONScontext* ctx) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    GLFONSbuffer* buffer = glfons__bufferBound(gl);

    gl->params.vertexData(gl->userPtr, buffer->nbVerts, buffer->interleavedArray);

    free(buffer->interleavedArray);
    buffer->interleavedArray = nullptr;
}

void glfonsTransform(FONScontext* ctx, fsuint id, float tx, float ty, float r, float a) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    GLFONSbuffer* buffer = glfons__bufferBound(gl);

    int i, j;

    // TODO : manage out of screen translations

    glfons__id2ij(gl, id, &i, &j);

    // scaling to [0..255]
    tx = (tx * 255.0) / gl->screenSize[0];
    ty = (ty * 255.0) / gl->screenSize[1];

    r = (r / (2.0 * M_PI)) * 255.0;
    a = a * 255.0;

    // scale decimal part from [0..1] to [0..255] rounding to the closest value
    // known floating point error here of 0.5/255 ~= 0.00196 due to rounding
    float dx = floor((1.0 - ((int)(tx + 1) - tx)) * 255.0 + 0.5);
    float dy = floor((1.0 - ((int)(ty + 1) - ty)) * 255.0 + 0.5);

    unsigned int data = glfonsRGBA(tx, ty, r, a);
    unsigned int fract = glfonsRGBA(dx, dy, /* bytes not used -> */ 0, 0);
    unsigned int index = j*buffer->transformRes[0]+i;

    if(data != buffer->transformData[index] || fract != buffer->transformData[index+1]) {
        buffer->transformData[index] = data;
        buffer->transformData[index+1] = fract;
        buffer->transformDirty[j] = 1;
    }
}

int glfonsGetGlyphCount(FONScontext* ctx, fsuint id) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    GLFONSbuffer* buffer = glfons__bufferBound(gl);

    if(buffer->stashes->find(id) != buffer->stashes->end()) {
        GLFONSstash* stash = buffer->stashes->at(id);
        return stash->nbGlyph;
    }
    
    return -1;
}

float glfonsGetGlyphOffset(FONScontext* ctx, fsuint id, int i) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    GLFONSbuffer* buffer = glfons__bufferBound(gl);
    GLFONSstash* stash = buffer->stashes->at(id);

    return stash->glyphsXOffset[i];
}

void glfonsGetBBox(FONScontext* ctx, fsuint id, float* x0, float* y0, float* x1, float* y1) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    GLFONSbuffer* buffer = glfons__bufferBound(gl);
    GLFONSstash* stash = buffer->stashes->at(id);
    
    *x0 = stash->bbox[0]; *y0 = stash->bbox[1];
    *x1 = stash->bbox[2]; *y1 = stash->bbox[3];
}

float glfonsGetLength(FONScontext* ctx, fsuint id) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    GLFONSbuffer* buffer = glfons__bufferBound(gl);
    GLFONSstash* stash = buffer->stashes->at(id);
    
    return stash->length;
}

#endif
