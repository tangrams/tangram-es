#pragma once

#include "texture.h"
#include "fontContext.h"
#include "glfontstash.h"
#include "stl_util.hpp"

class TextBuffer {

public:
    TextBuffer(FONScontext* _fsContext);
    TextBuffer(FONScontext* _fsContext, int _size);
    ~TextBuffer();

    fsuint genTextID();
    void rasterize(const std::string& _text, fsuint _id);
    void transformID(fsuint _textID, float _x, float _y, float _rot, float _alpha);
    void triggerTransformUpdate();

    void setTextureTransform(std::unique_ptr<Texture> _texture);
    const std::unique_ptr<Texture>& getTextureTransform() const;

private:
    bool validateBinding();
    void bind();
    void unbind();

    bool m_dirty;
    bool m_bound;
    std::unique_ptr<Texture> m_transform;
    fsuint m_fsBuffer;
    FONScontext* m_fsContext;

    static std::unique_ptr<std::mutex> s_contextMutex; 

};
