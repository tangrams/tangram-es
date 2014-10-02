/*
...
*/
#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "glm/glm.hpp"

#include "dataSource/dataSource.h"
#include "util/vertexLayout.h"
#include "util/shaderProgram.h"
#include "util/vboMesh.h"
#include "util/projection.h"
#include "platform.h"
#include "mapTile/mapTile.h"

class Style {
protected:
    std::string m_geomType;
    std::string m_styleName;

    std::vector<GLushort> m_indices;
    //What layer names will be handled by this Style
    std::unordered_map<std::string, glm::vec4> m_layerColorMap;
    
    std::shared_ptr<ShaderProgram> m_shaderProgram;
    std::shared_ptr<VertexLayout> m_vertexLayout;
    
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
     * addData uses the jsonRoot for a tile and iterate through all features calls fills the vbo with appropriate data.
     * VboMesh (in the MapTile _tile object) is updated with the style data and vertex information.
     */
    virtual void addData(Json::Value _jsonRoot, MapTile& _tile, MapProjection& _mapProjection) = 0;
    
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
    }
};

class PolygonStyle : public Style {
    
    struct vboDataUnit {
        //TODO: See how can we use glm::dvec3 here.
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
    
    virtual void addData(Json::Value _jsonRoot, MapTile& _tile, MapProjection& _mapProjection);
    virtual void constructVertexLayout();
    virtual void constructShaderProgram();
    
    virtual ~PolygonStyle() {
        m_vertices.clear();
    }
};

