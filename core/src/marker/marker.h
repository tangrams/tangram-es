#pragma once

#include "glm/mat4x4.hpp"
#include "glm/vec2.hpp"
#include "util/ease.h"
#include "util/geom.h"
#include <memory>
#include <string>

namespace Tangram {

class MapProjection;
class Scene;
class View;
struct DrawRule;
struct DrawRuleData;
struct Feature;
struct StyledMesh;

class Marker {

public:

    Marker();

    ~Marker();

    void setBounds(BoundingBox bounds);

    void setFeature(std::unique_ptr<Feature> feature);

    void setStyling(std::unique_ptr<DrawRuleData> drawRuleData);

    void setMesh(uint32_t styleId, std::unique_ptr<StyledMesh> mesh);

    void setEase(const glm::dvec2& destination, float duration, EaseType ease);

    void update(float dt, const View& view);

    uint32_t styleId() const;

    StyledMesh* mesh() const;

    // MapProjection* mapProjection() const;

    DrawRule* drawRule();

    Feature* feature() const;

    const BoundingBox& bounds() const;

    const glm::dvec2& origin() const;

    const glm::mat4& modelMatrix() const;

    bool isEasing() const;

protected:

    std::unique_ptr<Feature> m_feature;
    std::unique_ptr<StyledMesh> m_mesh;
    std::unique_ptr<DrawRuleData> m_drawRuleData;
    std::unique_ptr<DrawRule> m_drawRule;

    MapProjection* m_mapProjection = nullptr;

    uint32_t m_styleId = 0;

    // Origin of marker geometry relative to global projection space.
    glm::dvec2 m_origin;

    // Bounding box in global projection space which describes the origin and extent of the coordinates in the Feature
    BoundingBox m_bounds;

    // Matrix relating marker-local coordinates to global projection space coordinates;
    // Note that this matrix does not contain the relative translation from the global origin to the marker origin.
    // Distances from the global origin are too large to represent precisely in 32-bit floats, so we only apply the
    // relative translation from the view origin to the model origin immediately before drawing the marker.
    glm::mat4 m_modelMatrix;

    Ease m_ease;

};

} // namespace Tangram
