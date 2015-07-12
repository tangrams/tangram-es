#pragma once

#include "style.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "util/typedMesh.h"

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
    virtual void buildPoint(Point& _point, const StyleParamMap& _styleParamMap, Properties& _props, VboMesh& _mesh) const override;
    virtual void buildLine(Line& _line, const StyleParamMap& _styleParamMap, Properties& _props, VboMesh& _mesh) const override;
    virtual void buildPolygon(Polygon& _polygon, const StyleParamMap& _styleParamMap, Properties& _props, VboMesh& _mesh) const override;
    virtual void addData(TileData& _data, MapTile& _tile) override;

    virtual void parseStyleParams(const StyleParamMap& _styleParamMap, void* _styleParams) const override;

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
