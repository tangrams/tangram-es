#pragma once

#include "style.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "gl/typedMesh.h"

class Texture;

class SpriteStyle : public Style {

protected:

    struct PosUVVertex {
        // Position Data
        glm::vec3 pos;
        // UV Data
        glm::vec2 uv;
    };

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void buildPoint(Point& _point, void* _styleParam, Properties& _props, VboMesh& _mesh) const override;
    virtual void buildLine(Line& _line, void* _styleParam, Properties& _props, VboMesh& _mesh) const override;
    virtual void buildPolygon(Polygon& _polygon, void* _styleParam, Properties& _props, VboMesh& _mesh) const override;
    virtual void addData(TileData& _data, Tile& _tile) override;

    virtual void* parseStyleParams(const std::string& _layerNameID, const StyleParamMap& _styleParamMap) override;

    typedef TypedMesh<PosUVVertex> Mesh;

    virtual VboMesh* newMesh() const override {
        return nullptr;
    };

    std::shared_ptr<Texture> m_texture;

public:

    virtual void onBeginDrawFrame(const std::shared_ptr<View>& _view, const std::shared_ptr<Scene>& _scene) override;

    SpriteStyle(std::string _name, GLenum _drawMode = GL_TRIANGLES);

    virtual ~SpriteStyle();

};
