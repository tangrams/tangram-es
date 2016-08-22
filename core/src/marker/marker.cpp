#include "marker/marker.h"
#include "data/tileData.h"
#include "scene/drawRule.h"
#include "scene/scene.h"

#include "style/style.h"
#include "view/view.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"

namespace Tangram {

Marker::Marker() {
}

Marker::~Marker() {
}

void Marker::setBounds(BoundingBox bounds) {
    m_bounds = bounds;
    m_origin = bounds.min; // South-West corner
}

void Marker::setFeature(std::unique_ptr<Feature> feature) {
    m_feature = std::move(feature);
}

void Marker::setStyling(std::unique_ptr<DrawRuleData> drawRuleData) {
    m_drawRuleData = std::move(drawRuleData);
    m_drawRule = std::make_unique<DrawRule>(*m_drawRuleData, "anonymous_marker_layer", 0);
}

void Marker::setMesh(uint32_t styleId, std::unique_ptr<StyledMesh> mesh) {
    m_mesh = std::move(mesh);
    m_styleId = styleId;
}

void Marker::setEase(const glm::dvec2& dest, float duration, EaseType e) {
    auto origin = m_origin;
    auto cb = [=](float t) { m_origin = { ease(origin.x, dest.x, t, e), ease(origin.y, dest.y, t, e) }; };
    m_ease = { duration, cb };
}

void Marker::update(float dt, const View& view) {
    // Update easing
    if (!m_ease.finished()) { m_ease.update(dt); }
    // Apply marker-view translation to the model matrix
    const auto& viewOrigin = view.getPosition();
    auto scaling = glm::scale(glm::vec3(m_bounds.width(), m_bounds.height(), 1.f));
    auto translation = glm::translate(glm::vec3(m_origin.x - viewOrigin.x, m_origin.y - viewOrigin.y, 0.f));
    m_modelMatrix = translation * scaling;
}

uint32_t Marker::styleId() const {
    return m_styleId;
}

Feature* Marker::feature() const {
    return m_feature.get();
}

DrawRule* Marker::drawRule() {
    return m_drawRule.get();
}

StyledMesh* Marker::mesh() const {
    return m_mesh.get();
}

// MapProjection* Marker::mapProjection() const {
//     return m_mapProjection;
// }

const BoundingBox& Marker::bounds() const {
    return m_bounds;
}

const glm::dvec2& Marker::origin() const {
    return m_origin;
}

const glm::mat4& Marker::modelMatrix() const {
    return m_modelMatrix;
}

bool Marker::isEasing() const {
    return !m_ease.finished();
}

} // namespace Tangram
