#pragma once

#include "texture.h"
#include "glfontstash.h"

/* 
 * This class represents a text buffer, each text buffer has several text ids
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
    void init();

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
    void pushBuffer();
    
    /* 
     * fills the vector of float with the rasterized text ids linked to the text buffer 
     * nVerts is the number of vertices inside the vector
     */
    bool getVertices(float* _vertices);
    
    int getVerticesSize();

    /* get the axis aligned bounding box for a text */
    glm::vec4 getBBox(fsuint _textID);
    
    void bind();
    void unbind();

private:
    bool m_dirty;
    fsuint m_fsBuffer;
    FONScontext* m_fsContext;

};
