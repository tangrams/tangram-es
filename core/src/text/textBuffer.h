#pragma once

#include "gl.h"
#include "glfontstash.h"
#include "glm/vec4.hpp"

class Texture;
class VboMesh;

/* 
 * This class represents a text buffer, each text buffer has several text ids
 */
class TextBuffer {

public:

    TextBuffer(FONScontext* _fsContext);
    ~TextBuffer();
    
    /* generates a text id */
    fsuint genTextID();

    void init();

    /* ask the font rasterizer to rasterize a specific text for a text id */
    bool rasterize(const std::string& _text, fsuint _id);

    /* 
     * transform a text id in screen space coordinate
     *  x, y in screen space
     *  rotation is in radians
     *  alpha should be in [0..1]
     */
    void transformID(fsuint _textID, float _x, float _y, float _rot, float _alpha);
    
    void setMesh(std::shared_ptr<VboMesh> _mesh) { m_mesh = _mesh; }
    
    std::shared_ptr<VboMesh> getWeakMesh();

    void pushBuffer();
    
    /* 
     * fills the vector of float with the rasterized text ids linked to the text buffer 
     * nVerts is the number of vertices inside the vector
     */
    bool getVertices(float* _vertices);
    
    int getVerticesSize();
    
    bool hasData();
    
    /* get the axis aligned bounding box for a text */
    glm::vec4 getBBox(fsuint _textID);
    
    void bind();
    void unbind();

private:
    bool m_dirty;
    fsuint m_fsBuffer;
    FONScontext* m_fsContext;
    std::weak_ptr<VboMesh> m_mesh;

};
