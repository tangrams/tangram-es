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
            m_valid = true;
        }

        inline void operator()(const typename T::Type& _value) {
            if (m_current != _value || !m_valid) {
                m_current = _value;
                T::set(m_current);
            }
        }
        
        void invalidate() {
            m_valid = false;
        }

    private:
        bool m_valid;
        typename T::Type m_current;
    };

    struct DepthTest {
        using Type = GLboolean;
        inline static void set(const Type& _type) {
            if (_type) {
                glEnable(GL_DEPTH_TEST);
            } else {
                glDisable(GL_DEPTH_TEST);
            }
        }
    };

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

    struct Blending {
        using Type = GLboolean;
        inline static void set(const Type& _type) {
            if (_type) {
                glEnable(GL_BLEND);
            } else {
                glDisable(GL_BLEND);
            }
        }
    };

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
            }
        }
    };

    extern State<DepthTest> depthTest;
    extern State<DepthWrite> depthWrite;
    extern State<Blending> blending;
    extern State<BlendingFunc> blendingFunc;
    extern State<Culling> culling;

    void configure();
    void invalidateAllStates();
}

