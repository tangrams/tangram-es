#pragma once

#include <map>
#include "gl.h"
#include "platform.h"

namespace RenderState {

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
                glEnable(N);
            } else {
                glDisable(N);
            }
        }
    };

    using DepthTest = BoolSwitch<GL_DEPTH_TEST>;

    struct DepthWrite {
        using Type = GLboolean;
        inline static void set(const Type& _type) {
            if (_type) {
                glDepthMask(GL_TRUE);
            } else {
                glDepthMask(GL_FALSE);
            }
        }
    };

    struct ColorWrite {
        using Type = GLboolean;
        inline static void set(const Type& _type) {
            if (_type) {
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            } else {
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            }
        }
    };

    using Blending = BoolSwitch<GL_BLEND>;

    struct BlendingFunc {
        struct Type {
            GLenum src;
            GLenum dst;
            inline bool operator!=(const Type& _other) {
                return src != _other.src || dst != _other.dst;
            }
        };
        inline static void set(const Type& _type) {
            glBlendFunc(_type.src, _type.dst);
        }
    };

    struct Culling {
        struct Type {
            GLboolean culling;
            GLenum frontFaceOrder;
            GLenum face;
            inline bool operator!=(const Type& _other) {
                return culling != _other.culling || frontFaceOrder != _other.frontFaceOrder || face != _other.face;
            }
        };
        inline static void set(const Type& _type) {
            if (_type.culling) {
                glEnable(GL_CULL_FACE);
                glFrontFace(_type.frontFaceOrder);
                glCullFace(_type.face);
            } else {
                glDisable(GL_CULL_FACE);
            }
        }
    };

    using StencilTest = BoolSwitch<GL_STENCIL_TEST>;

    struct StencilWrite {
        using Type = GLuint;
        inline static void set(const Type& _type) {
            glStencilMask(_type);
        }
    };

    struct StencilFunc {
        struct Type {
            GLenum func;
            GLint ref;
            GLuint mask;
            inline bool operator!=(const Type& _other) {
                return func != _other.func || ref != _other.ref || mask != _other.mask;
            }
        };
        inline static void set(const Type& _type) {
            glStencilFunc(_type.func, _type.ref, _type.mask);
        }
    };

    struct StencilOp {
        struct Type {
            GLenum sfail;
            GLint dfail;
            GLuint dppass;
            inline bool operator!=(const Type& _other) {
                return sfail != _other.sfail || dfail != _other.dfail || dppass != _other.dppass;
            }
        };
        inline static void set(const Type& _type) {
            glStencilOp(_type.sfail, _type.sfail, _type.dppass);
        }
    };

    extern State<DepthTest> depthTest;
    extern State<DepthWrite> depthWrite;
    extern State<ColorWrite> colorWrite;
    extern State<Blending> blending;
    extern State<BlendingFunc> blendingFunc;
    extern State<Culling> culling;
    extern State<StencilTest> stencilTest;
    extern State<StencilWrite> stencilWrite;
    extern State<StencilFunc> stencilFunc;
    extern State<StencilOp> stencilOp;

    void configure();
}
