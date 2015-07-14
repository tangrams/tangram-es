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
            bool operator!=(const Type& _other) {
                return src == _other.src && dst != _other.dst;
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
        };
        inline static void set(const Type& _type) {
            if (_type.culling) {
                glEnable(GL_CULL_FACE);
                glFrontFace(_type.frontFaceOrder);
                glCullFace(_type.face);
            }
        }
    };
    
    inline bool operator!=(const Culling::Type& _lhs, const Culling::Type& _rhs) {
        return _lhs.culling == _rhs.culling && _lhs.frontFaceOrder == _rhs.frontFaceOrder && _lhs.face == _rhs.face;
    }
    
    static State<DepthTest> depthTest;
    static State<DepthWrite> depthWrite;
    static State<Blending> blending;
    static State<BlendingFunc> blendingFunc;
    static State<Culling> culling;
    
    void configure();
}

