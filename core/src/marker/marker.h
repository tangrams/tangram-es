#pragma once

#include "glm/mat4x4.hpp"
#include "glm/vec2.hpp"
#include "util/ease.h"
#include "util/geom.h"
#include "util/types.h"
#include <memory>
#include <string>

namespace Tangram {

class MapProjection;
class Scene;
class Texture;
class View;
struct DrawRule;
struct DrawRuleData;
struct Feature;
struct StyledMesh;

class Marker {

public:

    // Create an empty marker with the given ID. An ID of 0 indicates an invalid marker.
    Marker(MarkerID id);

    ~Marker();

    // Set the axis-aligned bounding box for the feature geometry in Mercator meters;
    // The points in the feature for this Marker should be made relative to a coordinate system
    // whose origin is the South-West corner of the bounds and whose unit length is the
    // maximum dimension (extent) of the bounds.
    void setBounds(BoundingBox bounds);

    // Set the feature whose geometry will be used to build the marker.
    void setFeature(std::unique_ptr<Feature> feature);

    // Set the string of YAML that will be used to style the marker.
    void setStylingString(std::string stylingString);

    // Set the draw rule that will be used to build the marker.
    void setDrawRule(std::unique_ptr<DrawRuleData> drawRuleData);

    // Set the styled mesh for this marker with the associated style id and zoom level.
    void setMesh(uint32_t styleId, uint32_t zoom, std::unique_ptr<StyledMesh> mesh);

    void setTexture(std::unique_ptr<Texture> texture);

    // Set an ease for the origin of this marker in Mercator meters.
    void setEase(const glm::dvec2& destination, float duration, EaseType ease);

    void setSelectionColor(uint32_t selectionColor);

    // Set the model matrix for the marker using the current view and update any eases.
    void update(float dt, const View& view);

    // Set whether this marker should be visible.
    void setVisible(bool visible);

    // Set the ordering of this marker relative to other markers.
    // Markers with higher values are drawn 'above' those with lower values.
    void setDrawOrder(int drawOrder);

    // Get the unique identifier for this marker. An ID of 0 indicates an invalid marker.
    MarkerID id() const;

    // Get the ID of the style that should draw the mesh for this marker.
    uint32_t styleId() const;

    // Get the zoom level at which the mesh for this marker was built.
    int builtZoomLevel() const;

    // Get the ordering of this marker relative to other markers.
    int drawOrder() const;

    // Get the length of the maximum dimension of the bounds of this marker. This is used as
    // the scale in the model matrix.
    float extent() const;

    StyledMesh* mesh() const;

    DrawRule* drawRule() const;

    Feature* feature() const;

    Texture* texture() const;

    const BoundingBox& bounds() const;

    // Get the origin of the geometry for this marker, i.e. the South-West corner of the bounds.
    // This is used as the origin in the model matrix.
    const glm::dvec2& origin() const;

    const glm::mat4& modelMatrix() const;

    const glm::mat4& modelViewProjectionMatrix() const;

    const std::string& stylingString() const;

    bool isEasing() const;

    bool isVisible() const;

    uint32_t selectionColor() const;

    static bool compareByDrawOrder(const std::unique_ptr<Marker>& lhs, const std::unique_ptr<Marker>& rhs);

protected:

    std::unique_ptr<Feature> m_feature;
    std::unique_ptr<StyledMesh> m_mesh;
    std::unique_ptr<DrawRuleData> m_drawRuleData;
    std::unique_ptr<DrawRule> m_drawRule;
    std::unique_ptr<Texture> m_texture;

    std::string m_stylingString;

    MarkerID m_id = 0;

    uint32_t m_styleId = 0;

    int m_builtZoomLevel = 0;

    uint32_t m_selectionColor = 0;

    int m_drawOrder = 0;

    // Origin of marker geometry relative to global projection space.
    glm::dvec2 m_origin;

    // Bounding box in global projection space which describes the origin and extent of the coordinates in the Feature
    BoundingBox m_bounds;

    // Matrix relating marker-local coordinates to global projection space coordinates;
    // Note that this matrix does not contain the relative translation from the global origin to the marker origin.
    // Distances from the global origin are too large to represent precisely in 32-bit floats, so we only apply the
    // relative translation from the view origin to the model origin immediately before drawing the marker.
    glm::mat4 m_modelMatrix;

    glm::mat4 m_modelViewProjectionMatrix;

    Ease m_ease;

    bool m_visible = true;

};

} // namespace Tangram
