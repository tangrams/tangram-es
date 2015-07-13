#pragma once

#include "gl.h"
#include "style/styleBatch.h"
#include "style/styleParamMap.h"
#include "data/tileData.h"
#include "util/typedMesh.h"
#include "style/polygonStyle.h"
#include "style/meshBatch.h"

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

class MapTile;

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

class PolygonBatch : public MeshBatch<PolygonStyle, Mesh> {

public:

    PolygonBatch(const PolygonStyle& _style);

    virtual void add(const Feature& _feature, const StyleParamMap& _params, const MapTile& _tile) override;

private:

    void buildLine(const Line& _line, const Properties& _props, const PolygonStyle::StyleParams& _params, const MapTile& _tile);
    void buildPolygon(const Polygon& _polygon, const Properties& _props, const PolygonStyle::StyleParams& _params, const MapTile& _tile);
};
