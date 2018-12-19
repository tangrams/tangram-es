#pragma once

#include "util/ease.h"
#include "util/geom.h"
#include "util/types.h"

#include "glm/mat4x4.hpp"
#include "glm/vec2.hpp"
#include <memory>
#include <string>

namespace Tangram {

class SceneLayer;
class DrawRuleMergeSet;
class MapProjection;
class Scene;
class StyleContext;
class Texture;
class View;
struct DrawRule;
struct DrawRuleData;
struct Feature;
struct StyledMesh;

class Marker {

    struct Styling {
        std::string string;
        bool isPath = false; // True if styling string is a path to a draw rule.
    };

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

    // Sets the styling struct for the marker
    void setStyling(std::string styling, bool isPath);

    // Set the new draw rule data that will be used to build the marker.
    void setDrawRuleData(std::unique_ptr<DrawRuleData> drawRuleData);

    // Merge draw rules from the given layer into the internal draw rule set.
    void mergeRules(const SceneLayer& layer);

    // From the set of merged draw rules, set the one with the given name as the
    // Marker's draw rule and clear the internal draw rule set. Returns true if
    // the named rule was found.
    bool finalizeRuleMergingForName(const std::string& name);

    // Set the styled mesh for this marker with the associated style id and zoom level.
    void setMesh(uint32_t styleId, uint32_t zoom, std::unique_ptr<StyledMesh> mesh);

    void clearMesh();

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

    // Get the length of the maximum dimension of the bounds of this marker.
    // This is used to calculate modelScale.
    float extent() const;

    // Get the scale for the model matrix. Depends on extent().
    float modelScale() const;

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

    const Styling& styling() const { return m_styling; }

    bool evaluateRuleForContext(StyleContext& ctx);

    bool isEasing() const;

    bool isVisible() const;

    uint32_t selectionColor() const;

    static bool compareByDrawOrder(const std::unique_ptr<Marker>& lhs, const std::unique_ptr<Marker>& rhs);

protected:

    std::unique_ptr<Feature> m_feature;
    std::unique_ptr<StyledMesh> m_mesh;
    std::unique_ptr<Texture> m_texture;
    std::unique_ptr<DrawRuleMergeSet> m_drawRuleSet;
    std::unique_ptr<DrawRuleData> m_drawRuleData;
    std::unique_ptr<DrawRule> m_drawRule;

    Styling m_styling;

    MarkerID m_id = 0;

    uint32_t m_styleId = 0;

    int m_builtZoomLevel = -1;

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
