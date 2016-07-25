#pragma once

#include "gl.h"
#include <array>

namespace Tangram {

class RenderState {

public:

    static constexpr size_t MAX_ATTRIBUTES = 16;

    // Reset the render states.
    void invalidate();

    int generation();

    void increaseGeneration();

    bool isValidGeneration(int _generation);

    // Get the texture slot from a texture unit from 0 to TANGRAM_MAX_TEXTURE_UNIT-1.
    static GLuint getTextureUnit(GLuint _unit);

    // Get the currently active texture unit.
    int currentTextureUnit();

    // Get the immediately next available texture unit and mark it unavailable.
    int nextAvailableTextureUnit();

    // Reset the currently used texture unit.
    void resetTextureUnit();

    // Release one texture unit slot.
    void releaseTextureUnit();

    bool blending(GLboolean enable);

    bool blendingFunc(GLenum sfactor, GLenum dfactor);

    bool clearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a);

    bool colorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a);

    bool cullFace(GLenum face);

    bool culling(GLboolean enable);

    bool depthTest(GLboolean enable);

    bool depthMask(GLboolean enable);

    bool frontFace(GLenum face);

    bool stencilMask(GLuint mask);

    bool stencilFunc(GLenum func, GLint ref, GLuint mask);

    bool stencilOp(GLenum sfail, GLenum spassdfail, GLenum spassdpass);

    bool stencilTest(GLboolean enable);

    bool shaderProgram(GLuint program);

    bool texture(GLenum target, GLuint handle);

    bool textureUnit(GLuint unit);

    bool vertexBuffer(GLuint handle);

    bool indexBuffer(GLuint handle);

    void vertexBufferUnset(GLuint handle);

    void indexBufferUnset(GLuint handle);

    void shaderProgramUnset(GLuint program);

    void textureUnset(GLenum target, GLuint handle);

    std::array<GLuint, MAX_ATTRIBUTES> attributeBindings = { { 0 } };

private:

    int m_validGeneration = 0;
    int m_nextTextureUnit = 0;

    struct {
        GLboolean enabled = 0;
        bool set = false;
    } m_blending, m_culling, m_depthMask, m_depthTest, m_stencilTest;

    struct {
        GLenum sfactor = 0, dfactor = 0;
        bool set = false;
    } m_blendingFunc;

    struct {
        GLuint mask = 0;
        bool set = false;
    } m_stencilMask;

    struct {
        GLenum func = 0;
        GLint ref = 0;
        GLuint mask = 0;
        bool set = false;
    } m_stencilFunc;

    struct {
        GLenum sfail = 0, spassdfail = 0, spassdpass = 0;
        bool set = false;
    } m_stencilOp;

    struct {
        GLboolean r = 0, g = 0, b = 0, a = 0;
        bool set = false;
    } m_colorMask;

    struct {
        GLenum face = 0;
        bool set = false;
    } m_frontFace, m_cullFace;

    struct {
        GLuint handle = 0;
        bool set = false;
    } m_vertexBuffer, m_indexBuffer;

    struct {
        GLuint program = 0;
        bool set = false;
    } m_program;

    struct {
        GLclampf r = 0., g = 0., b = 0., a = 0.;
        bool set = false;
    } m_clearColor;

    struct {
        GLenum target = 0;
        GLuint handle = 0;
        bool set = false;
    } m_texture;

    struct {
        GLuint unit = 0;
        bool set = false;
    } m_textureUnit;

};

}
