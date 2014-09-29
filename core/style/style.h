/*
...
*/
#pragma once

#include <string>
#include <vector>

#include "tesselator.h"
#include "glm/glm.hpp"

#include "dataSource/dataSource.h"
#include "util/vertexLayout.h"
#include "util/shaderProgram.h"
#include "util/vboMesh.h"
#include "util/projection.h"
#include "platform.h"

class Style {
protected:
    std::string m_geomType;
    std::string m_styleName;

    std::vector<GLushort> m_indices;
    
    std::shared_ptr<ShaderProgram> m_shaderProgram;
    std::shared_ptr<VertexLayout> m_vertexLayout;
    
    TESStesselator* m_tess;

    GLenum m_drawMode;

    std::string m_fragShaderSrcStr;
    std::string m_vertShaderSrcStr;

public:
    Style(std::string _geomType, GLenum _drawMode);
    Style(std::string _geomType, std::string _styleName, GLenum _drawMode);
    
    /* 
     * Respnsible to create Vertex Layout corresponding to a style
     *  In future, maybe can be made non pure and implement it in the base class
     *  to read the "style description" and create a vertex layout based on that.
     */
    virtual void constructVertexLayout() = 0;

    /*
     * Responsible to create a shader program for this style.
     *  Uses the defined frag and vert member shader source.
     */
    virtual void constructShaderProgram() = 0;

    /*
     * addDataToVBO uses the Geometry and Property of a Json->Layer->Feature to tesselate the geometry
     *  and create vertex data. VboMesh is updated with the style data and vertex information.
     *  NOTE: _color is glm::vec3 type and hence floats (0.0-1.0). 
     *  These will be converted to GLubyte in the implementation
     */
    virtual void addDataToVBO(Json::Value _geomCoordinates, Json::Value _property, glm::vec4& _color, std::shared_ptr<VboMesh> _vboMesh, glm::vec2 _tileOffset, MapProjection& _mapProjection) = 0;
    
    /*
     * Sets fragment shader source
     */
    void setFragShaderSrc(std::string _fragShaderSrcStr);

    /*
     * Sets vertex shader source
     */
    void setVertShaderSrc(std::string _vertShaderSrcStr);

    // Returns the shader program to be fed to vboMesh->draw
    std::shared_ptr<ShaderProgram> getShaderProgram();
    std::string getStyleName();
    std::string getGeometryType();

    virtual ~Style() {
        m_indices.clear();
        tessDeleteTess(m_tess);
    }
};

class PolygonStyle : public Style {
    
    struct vboDataUnit {
        //TODO: See how can we use glm::vec3 here.
        //Position Data
        GLfloat pos_x;
        GLfloat pos_y;
        GLfloat pos_z;
        //Normal Data
        GLfloat norm_x;
        GLfloat norm_y;
        GLfloat norm_z;
        //Color Data
        // GLubyte rgba[4];
        GLubyte r;
        GLubyte g;
        GLubyte b;
        GLubyte a;
    };
    
    std::vector<vboDataUnit> m_vertices;

public:
 
    PolygonStyle(std::string _geomType, GLenum _drawMode = GL_TRIANGLES);
    PolygonStyle(std::string _geomType, std::string _styleName, GLenum _drawMode = GL_TRIANGLES);
    
    virtual void addDataToVBO(Json::Value _geomCoordinates, Json::Value _property, glm::vec4& _color, std::shared_ptr<VboMesh> _vboMesh, glm::vec2 _tileOffset, MapProjection& _mapProjection);
    virtual void constructVertexLayout();
    virtual void constructShaderProgram();
    
    virtual ~PolygonStyle() {
        m_vertices.clear();
    }
};

