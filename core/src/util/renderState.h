#pragma once

#include "gl.h"
#include "platform.h"

#include <tuple>

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

    // http://stackoverflow.com/questions/7858817/unpacking-a-tuple-to-call-a-matching-function-pointer

    template<int ...> struct seq {};
    template<int N, int ...S> struct gens : gens<N-1, N-1, S...> {};
    template<int ...S> struct gens<0, S...>{ typedef seq<S...> type; };

    template <typename ...Args>
    struct StateWrap {

        using Type = std::tuple<Args...>;
        Type params;
        void (*func)(Args...);

        StateWrap(void (*func)(Args...))
            : func(func) {}

        void init(const Args&... _param) {
            params = std::make_tuple(_param...);
        }

        void operator()(const Args&... _param) {
            auto _params = std::make_tuple(_param...);

            if (_params != params) {
                call(_params, typename gens<sizeof...(Args)>::type());
                params = _params;
            }
        }

        template<int ...S>
        void call(const Type& _params, seq<S...>) {
            func(std::get<S>(_params) ...);
        }

        inline bool operator!=(const Type& _other) {
            return params != _other;
        }
    };

    using DepthTest = BoolSwitch<GL_DEPTH_TEST>;
    extern State<DepthTest> depthTest;

    extern StateWrap<GLboolean> depthWrite;

    using Blending = BoolSwitch<GL_BLEND>;
    extern State<Blending> blending;

    extern StateWrap<GLenum, GLenum> blendingFunc;

    using StencilTest = BoolSwitch<GL_STENCIL_TEST>;
    extern State<StencilTest> stencilTest;

    extern StateWrap<GLuint> stencilWrite;
    extern StateWrap<GLenum, GLint, GLuint> stencilFunc;
    extern StateWrap<GLenum, GLenum, GLenum> stencilOp;

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

    extern State<ColorWrite> colorWrite;

    struct Culling  {
        struct Type {
            GLboolean culling;
            GLenum frontFaceOrder;
            GLenum face;
            inline bool operator!=(const Type& _other) {
                return culling != _other.culling ||
                    frontFaceOrder != _other.frontFaceOrder ||
                    face != _other.face;
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

    extern State<Culling> culling;


    void configure();
}
