#pragma once

#include "gl.h"
#include <array>
#include <string>
#include <mutex>
#include <vector>
#include <unordered_map>

namespace Tangram {

class Disposer;
class Scene;
class Texture;

class RenderState {

public:

    static constexpr size_t MAX_ATTRIBUTES = 16;

    static constexpr size_t MAX_QUAD_VERTICES = 16384;

    RenderState();
    ~RenderState();

    RenderState(const RenderState&) = delete;
    RenderState(RenderState&&) = delete;
    RenderState& operator=(const RenderState&) = delete;
    RenderState& operator=(RenderState&&) = delete;

    // Reset the GL state cache and resource handles.
    // Call this after GL context loss.
    void invalidate();

    // Reset the GL state cache.
    // Call this when outside code may have changed OpenGL states.
    void invalidateStates();

    // Reset the resource handle cache.
    void invalidateHandles();

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

    void defaultOpaqueClearColor(GLclampf r, GLclampf g, GLclampf b);

    bool defaultOpaqueClearColor();

    void clearDefaultOpaqueColor();

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

    void texture(GLuint handle, GLuint unit, GLenum target);

    bool vertexBuffer(GLuint handle);

    bool indexBuffer(GLuint handle);

    bool framebuffer(GLuint handle);

    bool viewport(GLint x, GLint y, GLsizei width, GLsizei height);

    void indexBufferUnset(GLuint handle);

    void cacheDefaultFramebuffer();

    GLuint defaultFrameBuffer() const;

    GLuint getQuadIndexBuffer();

    void flushResourceDeletion();

    void queueTextureDeletion(GLuint texture);

    void queueVAODeletion(size_t count, GLuint* vao);

    void queueBufferDeletion(size_t count, GLuint* buffers);

    void queueFramebufferDeletion(GLuint framebuffer);

    void queueProgramDeletion(GLuint program);

    std::array<GLuint, MAX_ATTRIBUTES> attributeBindings = { { 0 } };

    std::unordered_map<std::string, GLuint> fragmentShaders;
    std::unordered_map<std::string, GLuint> vertexShaders;

    float frameTime() { return m_frameTime; }

    friend class Scene;

protected:
    void setFrameTime(float _time) { m_frameTime = _time; }

private:

    float m_frameTime = 0.f;

    std::mutex m_deletionListMutex;
    std::vector<GLuint> m_VAODeletionList;
    std::vector<GLuint> m_bufferDeletionList;
    std::vector<GLuint> m_textureDeletionList;
    std::vector<GLuint> m_programDeletionList;
    std::vector<GLuint> m_shaderDeletionList;
    std::vector<GLuint> m_framebufferDeletionList;

    uint32_t m_nextTextureUnit = 0;

    GLuint m_quadIndexBuffer = 0;
    void deleteQuadIndexBuffer();
    void generateQuadIndexBuffer();

    struct {
        GLboolean enabled;
        bool set;
    } m_blending, m_culling, m_depthMask, m_depthTest, m_stencilTest;

    struct {
        GLenum sfactor, dfactor;
        bool set;
    } m_blendingFunc;

    struct {
        GLuint mask;
        bool set;
    } m_stencilMask;

    struct {
        GLenum func;
        GLint ref;
        GLuint mask;
        bool set;
    } m_stencilFunc;

    struct {
        GLenum sfail, spassdfail, spassdpass;
        bool set;
    } m_stencilOp;

    struct {
        GLboolean r, g, b, a;
        bool set;
    } m_colorMask;

    struct {
        GLenum face;
        bool set;
    } m_frontFace, m_cullFace;

    struct {
        GLuint handle;
        bool set;
    } m_vertexBuffer, m_indexBuffer;

    struct {
        GLuint program;
        bool set;
    } m_program;

    struct {
        GLclampf r, g, b, a;
        bool set;
    } m_clearColor;

    struct {
        GLclampf r, g, b;
        bool set;
    } m_defaultOpaqueClearColor;

    struct {
        GLenum target;
        GLuint handle;
        bool set;
    } m_texture;

    struct {
        GLuint unit;
        bool set;
    } m_textureUnit;

    struct FrameBufferState {
        GLuint handle;
        bool set;
    } m_framebuffer;

    struct ViewportState {
        GLint x;
        GLint y;
        GLsizei width;
        GLsizei height;
        bool set;
    } m_viewport;

    GLint m_defaultFramebuffer = 0;

};

}
