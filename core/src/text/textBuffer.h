#pragma once

#include "texture.h"
#include "fontContext.h"
#include "glfontstash.h"

class TextBuffer {

public:
    TextBuffer(FONScontext* _fsContext);
    TextBuffer(FONScontext* _fsContext, int _size);
    ~TextBuffer();

    void transformID(fsuint _textID, float _x, float _y, float _rot, float _alpha);
    void triggerTransformUpdate();

    const std::unique_ptr<Texture>& getTextureTransform() const;

private:
    void bind();
    void unbind();

    bool m_dirty;
    bool m_bound;
    std::unique_ptr<Texture> m_transform;
    fsuint m_fsBuffer;
    FONScontext* m_fsContext;

    static std::unique_ptr<std::mutex> m_contextMutex;

};
