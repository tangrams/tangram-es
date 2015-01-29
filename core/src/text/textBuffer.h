#pragma once

#include "texture.h"
#include "glfontstash.h"

class TextBuffer {

public:

    TextBuffer(FONScontext* _fsContext);
    ~TextBuffer();

    fsuint genTextID();
    void init(int _size = 2);
    void rasterize(const std::string& _text, fsuint _id);
    void transformID(fsuint _textID, float _x, float _y, float _rot, float _alpha);
    void triggerTransformUpdate();

    void setTextureTransform(std::unique_ptr<Texture> _texture);
    std::shared_ptr<Texture> getTextureTransform() const;
    bool getVertices(std::vector<float>* _vertices, int* _nVerts);
    void expand();

private:

    void bind();
    void unbind();

    bool m_dirty;
    bool m_bound;
    std::shared_ptr<Texture> m_transform;
    fsuint m_fsBuffer;
    FONScontext* m_fsContext;

};
