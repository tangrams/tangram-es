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

class MapTile;

class Style {
protected:
    /*
     * m_geomType represents the type of geometry in the geoJson, so this could be
     * - Polygon
     * - MultiPoint
     * - LineString
     * - MultilineString
     * - MultiPolygon, or
     * - GeometryCollection
     */
    std::string m_geomType;
    /*
     * m_styleName is a unique name for a style instance, which is checked/implemented in the sceneDirector
     * By default this takes the name of the geomType
     */
    std::string m_styleName;
    
    /* A collection of vertex indices maintained by this style to be fed to a vboMesh */
    std::vector<GLushort> m_indices;
    
    /* 
     * A collection of floats to be used to extract vertex coordinates from the geomHandler and feed to the vertexDataUnit specific to a style
     * NOTE: These will be cleared when a tile is done tesselating
     */
    std::vector<glm::vec3> m_vertCoords;
    
    /* 
     * A collection of floats to be used to extract normals for extruded vertices per vertex coordinate from the geomHandler and feed to the vertexDataUnit specific to a style
     * NOTE: These will be cleared when a tile is done tesselating
     * NOTE2: This might not be used by all styles
     */
    std::vector<glm::vec3> m_vertNormals;
    
    //What layer names will be handled by this Style
    std::unordered_map<std::string, GLuint> m_layerColorMap;
    
    /* 
     * shaderProgram, instantiated and could be shared between different styles
     * However, raw pointer can be accessed by other modules
     */
    std::shared_ptr<ShaderProgram> m_shaderProgram;
    /*
     * vertexLayout, instantiated by the style
     * shared between different meshes using this style
     */
    std::shared_ptr<VertexLayout> m_vertexLayout;
    
    /*
     * What draw mode to call GL with
     */
    GLenum m_drawMode;

    /* shader source */
    std::string m_fragShaderSrcStr;
    std::string m_vertShaderSrcStr;
    

public:

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
     * Method to fill the vertexDataUnit specific to every style based on the tesselated geometry
     */
    virtual void fillVertexData(const GLuint& _abgr) = 0;

    /* 
     * UpdateLayer updates the m_layerColorMap with a new definition of json layers which this style will process
     */
    virtual void updateLayers(std::vector<std::pair<std::string, GLuint>> _newLayers);
    
    /*
     * addData uses the jsonRoot for a tile and iterate through all features calls fills the vbo with appropriate data.
     * VboMesh (in the MapTile _tile object) is updated with the style data and vertex information.
     */
    virtual void addData(const Json::Value& _jsonRoot, MapTile& _tile, const MapProjection& _mapProjection) = 0;

    /*
     * clears m_indices and m_vertices. This needs to be called every time new tile is required to be filled with
     * vertices and indices.
     */
    virtual void clearStyleData() = 0;
    
    /*
     * Do any gl operation on the shader
     */
    virtual void setup() = 0;
    
    /*
     * Sets fragment shader source
     */
    void setFragShaderSrc(std::string _fragShaderSrcStr);

    /*
     * Sets vertex shader source
     */
    void setVertShaderSrc(std::string _vertShaderSrcStr);

    // Returns the shader program to be fed to vboMesh->draw
    std::shared_ptr<ShaderProgram> getShaderProgram() const { return m_shaderProgram; }
    std::string getStyleName() const { return m_styleName;}
    std::string getGeometryType() const { return m_geomType;}

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
        GLuint abgr;
    };
    
    /* A collection of vertices maintained by this style to be fed to a vboMesh */
    std::vector<vboDataUnit> m_vertices;

    virtual void constructVertexLayout();
    virtual void constructShaderProgram();
    virtual void fillVertexData(const GLuint& _abgr);

public:
 
    PolygonStyle(std::string _geomType, GLenum _drawMode = GL_TRIANGLES);
    PolygonStyle(std::string _geomType, std::string _styleName, GLenum _drawMode = GL_TRIANGLES);
    
    virtual void addData(const Json::Value& _jsonRoot, MapTile& _tile, const MapProjection& _mapProjection);
    virtual void setup();
    
    virtual void clearStyleData();
    
    virtual ~PolygonStyle() {
        m_vertices.clear();
    }
};

class MultiPolygonStyle : public Style {

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
        GLuint abgr;
    };
    
    /* A collection of vertices maintained by this style to be fed to a vboMesh */
    std::vector<vboDataUnit> m_vertices;

    virtual void constructVertexLayout();
    virtual void constructShaderProgram();
    virtual void fillVertexData(const GLuint& _abgr);

public:
 
    MultiPolygonStyle(std::string _geomType, GLenum _drawMode = GL_TRIANGLES);
    MultiPolygonStyle(std::string _geomType, std::string _styleName, GLenum _drawMode = GL_TRIANGLES);
    
    virtual void addData(const Json::Value& _jsonRoot, MapTile& _tile, const MapProjection& _mapProjection);
    virtual void setup();
    
    virtual void clearStyleData();
    
    virtual ~MultiPolygonStyle() {
        m_vertices.clear();
    }
};
