#pragma once

#include "gl.h"
#include "style/styleBatch.h"
#include "style/styleParamMap.h"
#include "data/tileData.h"
#include "util/typedMesh.h"
#include "style/polygonStyle.h"

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

class PolygonStyle;
class MapTile;
class View;

class PolygonBatch : public StyleBatch {

    struct PosNormColVertex {
        // Position Data
        glm::vec3 pos;
        // Normal Data
        glm::vec3 norm;
        // UV Data
        glm::vec2 texcoord;
        // Color Data
        GLuint abgr;
        // Layer Data
        GLfloat layer;
    };

    using Mesh = TypedMesh<PosNormColVertex>;

public:

    PolygonBatch(const PolygonStyle& _style);

    virtual void draw(const View& _view) override;
    virtual void update(const glm::mat4& mvp, const View& _view, float _dt) override {};
    virtual void prepare() override {};
    virtual bool compile() override;
    virtual void add(const Feature& _feature, const StyleParamMap& _params, const MapTile& _tile) override;

private:

    void buildLine(const Line& _line, const Properties& _props, const PolygonStyle::StyleParams& _params, const MapTile& _tile);
    void buildPolygon(const Polygon& _polygon, const Properties& _props, const PolygonStyle::StyleParams& _params, const MapTile& _tile);

    std::shared_ptr<Mesh> m_mesh;
    const PolygonStyle& m_style;
};
