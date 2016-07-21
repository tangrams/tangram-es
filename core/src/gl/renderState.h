#pragma once

#include "gl.h"
#include "gl/error.h"

#include <tuple>
#include <limits>

namespace Tangram {

class RenderState {

public:

    // Reset the render states.
    void invalidate();

    int generation();

    void increaseGeneration();

    bool isValidGeneration(int _generation);

    // Get the texture slot from a texture unit from 0 to TANGRAM_MAX_TEXTURE_UNIT-1.
    static GLuint getTextureUnit(GLuint _unit);

    // Bind a vertex buffer.
    static void bindVertexBuffer(GLuint _id);

    // Bind an index buffer.
    static void bindIndexBuffer(GLuint _id);

    // Bind a texture for the specified target.
    static void bindTexture(GLenum _target, GLuint _textureId);

    // Set the currently active texture unit.
    static void activeTextureUnit(GLuint _unit);

    // Get the currently active texture unit.
    int currentTextureUnit();

    // Get the immediately next available texture unit and mark it unavailable.
    int nextAvailableTextureUnit();

    // Reset the currently used texture unit.
    void resetTextureUnit();

    // Release one texture unit slot.
    void releaseTextureUnit();

    template <typename T>
    class State {
    public:
        void init(const typename T::Type& _default) {
            T::set(_default);
            m_current = _default;
        }

        inline void operator()(const typename T::Type& _value) {
            if (m_current != _value) {
                m_current = _value;
                T::set(m_current);
            }
        }
    private:
        typename T::Type m_current;
    };

    template <GLenum N>
    struct BoolSwitch {
        using Type = GLboolean;
        inline static void set(const Type& _type) {
            if (_type) {
                GL_CHECK(glEnable(N));
            } else {
                GL_CHECK(glDisable(N));
            }
        }
    };

    // http://stackoverflow.com/questions/7858817/unpacking-a-tuple-to-call-a-matching-function-pointer
    // Generate integer sequence for getting values from 'params' tuple.
    template<int ...> struct seq {};
    template<int N, int ...S> struct gens : gens<N-1, N-1, S...> {};
    template<int ...S> struct gens<0, S...>{ typedef seq<S...> type; };

    template <typename F, F fn, typename ...Args>
    struct StateWrap {

        using Type = std::tuple<Args...>;
        Type params;

        void init(Args... _param, bool _force = true) {
            params = std::make_tuple(_param...);
            if (_force) {
                call(typename gens<sizeof...(Args)>::type());
            }
        }

        inline void operator()(Args... _args) {
            auto _params = std::make_tuple(_args...);

            if (_params != params) {
                params = _params;
                call(typename gens<sizeof...(Args)>::type());
            }
        }

        inline bool compare(Args... _args) {
            auto _params = std::make_tuple(_args...);
            return _params == params;
        }

        template<int ...S>
        inline void call(seq<S...>) {
            GL_CHECK(fn(std::get<S>(params) ...));
        }
    };


    using DepthTest = State<BoolSwitch<GL_DEPTH_TEST>>;
    using StencilTest = State<BoolSwitch<GL_STENCIL_TEST>>;
    using Blending = State<BoolSwitch<GL_BLEND>>;
    using Culling = State<BoolSwitch<GL_CULL_FACE>>;

#define FUN(X) decltype((X)), X

    using DepthWrite = StateWrap<FUN(glDepthMask),
                                 GLboolean>; // enabled

    using BlendingFunc = StateWrap<FUN(glBlendFunc),
                                   GLenum,  // sfactor
                                   GLenum>; // dfactor

    using StencilWrite = StateWrap<FUN(glStencilMask),
                                   GLuint>; // mask

    using StencilFunc = StateWrap<FUN(glStencilFunc),
                                  GLenum,  // func
                                  GLint,   // ref
                                  GLuint>; // mask

    using StencilOp = StateWrap<FUN(glStencilOp),
                                GLenum,  // stencil:fail
                                GLenum,  // stencil:pass, depth:fail
                                GLenum>; // both pass

    using ColorWrite = StateWrap<FUN(glColorMask),
                                 GLboolean,  // red
                                 GLboolean,  // green
                                 GLboolean,  // blue
                                 GLboolean>; // alpha

    using FrontFace = StateWrap<FUN(glFrontFace),
                                GLenum>;

    using CullFace = StateWrap<FUN(glCullFace),
                               GLenum>;

    using VertexBuffer = StateWrap<FUN(bindVertexBuffer), GLuint>;
    using IndexBuffer = StateWrap<FUN(bindIndexBuffer), GLuint>;

    using ShaderProgram = StateWrap<FUN(glUseProgram), GLuint>;

    using TextureUnit = StateWrap<FUN(activeTextureUnit), GLuint>;
    using Texture = StateWrap<FUN(bindTexture), GLenum, GLuint>;

    using ClearColor = StateWrap<FUN(glClearColor),
                                 GLclampf,  // red
                                 GLclampf,  // green
                                 GLclampf,  // blue
                                 GLclampf>; // alpha

#undef FUN

    DepthTest depthTest;
    DepthWrite depthWrite;
    Blending blending;
    BlendingFunc blendingFunc;
    StencilTest stencilTest;
    StencilWrite stencilWrite;
    StencilFunc stencilFunc;
    StencilOp stencilOp;
    ColorWrite colorWrite;
    FrontFace frontFace;
    CullFace cullFace;
    Culling culling;
    ShaderProgram shaderProgram;
    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;
    TextureUnit textureUnit;
    Texture texture;
    ClearColor clearColor;

private:

    int m_validGeneration = 0;
    int m_textureUnit = -1;

};

}
