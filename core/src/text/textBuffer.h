#pragma once

#include "texture.h"
#include "glfontstash.h"
#include "util/typedMesh.h"

struct TextVert {
    glm::vec2 pos;
    glm::vec2 uvs;
    glm::vec2 screenPos;
    float alpha;
    float rotation;
};

class FontContext;

/*
 * This class represents a text buffer, each text buffer has several text ids
 */
class TextBuffer : public TypedMesh<TextVert> {

public:

    TextBuffer(std::shared_ptr<FontContext> _fontContext, std::shared_ptr<VertexLayout> _vertexLayout);
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
    
    /* get the vertices from the font context and add them as vbo mesh data */
    void finish();

private:
    bool m_dirty;
    fsuint m_fsBuffer;
    FONScontext* m_fsContext;

};
