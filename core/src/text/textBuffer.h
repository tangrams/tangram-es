#pragma once

#include "texture.h"
#include "glfontstash.h"

/* 
 * This class represents a text buffer, each text buffer has several text ids and a single 
 * transform texture. The transform texture is a texture containing the text ids transformations 
 * in screen space. 
 * There are three callbacks that the usage of this class could potentially trigger (those are 
 * defined in <FontContext>:
 *  - texture transform creation : lets you create the gpu transform texture, called after
 *  TextBuffer::init.
 *  - texture transform update : lets you update the gpu transform texture, called after 
 *  TextBuffer::triggerTransformUpdate.
 *  - an error callback : usually when you asked to generate too many text ids, this would lets 
 *  you expand the text buffer transform texture.
 *
 */
class TextBuffer {

public:

    TextBuffer(FONScontext* _fsContext);
    ~TextBuffer();
    
    /* generates a text id */
    fsuint genTextID();

    /* 
     * inits the text buffer, the size is the size of the transform texture, a size of 2 would 
     * create a transform texture of size 2x4.
     */
    void init(int _size = 2);

    /* ask the font rasterizer to rasterize a specific text for a text id */
    void rasterize(const std::string& _text, fsuint _id);

    /* 
     * transform a text id in screen space coordinate
     *  x, y should be inside the screen bounds
     *  rotation is in radians
     *  alpha should be in [0..1]
     */
    void transformID(fsuint _textID, float _x, float _y, float _rot, float _alpha);

    /* ask to update to update the transform texture related to this text buffer */
    void triggerTransformUpdate();

    /* sets the transform texture of this text buffer */
    void setTextureTransform(std::unique_ptr<Texture> _texture);

    /* get the related texture containing the transforms of text ids of this text buffer */
    std::shared_ptr<Texture> getTextureTransform() const;

    /* 
     * fills the vector of float with the rasterized text ids linked to the text buffer 
     * nVerts is the number of vertices inside the vector
     */
    bool getVertices(float* _vertices);
    
    int getVerticesSize();

    /* double the size of the related texture transform of the text buffer */ 
    void expand();
    
    /* get the axis aligned bounding box for a text */
    glm::vec4 getBBox(fsuint _textID);

private:

    void bind();
    void unbind();

    bool m_dirty;
    bool m_bound;
    std::shared_ptr<Texture> m_transform;
    fsuint m_fsBuffer;
    FONScontext* m_fsContext;

};
