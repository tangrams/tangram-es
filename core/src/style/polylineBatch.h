#pragma once

#include "style/styleBatch.h"
#include "style/polylineStyle.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "gl.h"
#include "tileData.h"
#include "util/typedMesh.h"

class PolylineStyle;

class PolylineBatch : public StyleBatch {

    struct PosNormEnormColVertex {
        //Position Data
        glm::vec3 pos;
        // UV Data
        glm::vec2 texcoord;
        // Extrude Normals Data
        glm::vec2 enorm;
        GLfloat ewidth;
        // Color Data
        GLuint abgr;
        // Layer Data
        GLfloat layer;
    };

    using Mesh = TypedMesh<PosNormEnormColVertex>;

public:
    PolylineBatch(const PolylineStyle& _style);

    virtual void draw(const View& _view) override;
    virtual void update(const glm::mat4& mvp, const View& _view, float _dt) override {};
    virtual void prepare() override {};
    virtual bool compile();

    virtual void add(const Feature& _feature, const StyleParamMap& _params, const MapTile& _tile) override;

private:
    void buildLine(const Line& _line, const Properties& _props, const PolylineStyle::StyleParams& _params, const MapTile& _tile);

    std::shared_ptr<Mesh> m_mesh;
    const PolylineStyle& m_style;
};
