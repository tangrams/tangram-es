#include "marker/marker.h"

#include "data/tileData.h"
#include "gl/texture.h"
#include "scene/dataLayer.h"
#include "scene/drawRule.h"
#include "scene/scene.h"
#include "style/style.h"
#include "view/view.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"

namespace Tangram {

Marker::Marker(MarkerID id) : m_id(id) {
    m_drawRuleSet.reset(new DrawRuleMergeSet());
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

void Marker::setStyling(std::string styling, bool isPath) {
    m_styling.string = styling;
    m_styling.isPath = isPath;
}

bool Marker::evaluateRuleForContext(StyleContext& ctx) {
    return m_drawRuleSet->evaluateRuleForContext(*drawRule(), ctx);
}

bool Marker::setDrawRule(std::unique_ptr<DrawRuleData> drawRuleData) {
    // clear previous set
    auto& drawRules = m_drawRuleSet->matchedRules();
    drawRules.clear();

    m_drawRuleData = std::move(drawRuleData);
    drawRules.emplace_back(*m_drawRuleData, "", 0);
    return true;
}

bool Marker::setDrawRule(const std::vector<const SceneLayer*>& layers) {
    // clear previous set
    auto& drawRules = m_drawRuleSet->matchedRules();
    drawRules.clear();

    m_drawRuleSet->mergeRules(layers);
    if (drawRules.empty()) { return false; }

    return true;
}

void Marker::setMesh(uint32_t styleId, uint32_t zoom, std::unique_ptr<StyledMesh> mesh) {
    m_mesh = std::move(mesh);
    m_styleId = styleId;
    m_builtZoomLevel = zoom;

    float scale;
    if (m_feature && m_feature->geometryType == GeometryType::points) {
        scale = (MapProjection::HALF_CIRCUMFERENCE * 2) / (1 << zoom);
    } else {
        scale = extent();
    }
    m_modelMatrix = glm::scale(glm::vec3(scale));
}

void Marker::setTexture(std::unique_ptr<Texture> texture) {
    m_texture = std::move(texture);
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
    m_modelMatrix[3][0] = m_origin.x - viewOrigin.x;
    m_modelMatrix[3][1] = m_origin.y - viewOrigin.y;

    m_modelViewProjectionMatrix = view.getViewProjectionMatrix() * m_modelMatrix;
}

void Marker::setVisible(bool visible) {
    m_visible = visible;
}

void Marker::setDrawOrder(int drawOrder) {
    m_drawOrder = drawOrder;
}

void Marker::setSelectionColor(uint32_t selectionColor) {
    m_selectionColor = selectionColor;
}

int Marker::builtZoomLevel() const {
    return m_builtZoomLevel;
}

int Marker::drawOrder() const {
    return m_drawOrder;
}

MarkerID Marker::id() const {
    return m_id;
}

uint32_t Marker::styleId() const {
    return m_styleId;
}

float Marker::extent() const {
    return glm::max(m_bounds.width(), m_bounds.height());
}

Feature* Marker::feature() const {
    return m_feature.get();
}

DrawRule* Marker::drawRule() const {
    auto& matchedRules = m_drawRuleSet->matchedRules();

    if (matchedRules.empty()) { return nullptr; }
    if (!m_styling.isPath) { return &matchedRules.front(); }

    auto n = m_styling.string.rfind(LAYER_DELIMITER);
    if (n == std::string::npos) { return nullptr; }
    auto drawRuleGrp = m_styling.string.substr(n+1);

    // TODO: Draw markers with multiple styles
    for (auto& matchedRule : matchedRules) {
        if (*(matchedRule.name) == drawRuleGrp) {
            return &matchedRule;
        }
    }
    return nullptr;
}

StyledMesh* Marker::mesh() const {
    return m_mesh.get();
}

Texture* Marker::texture() const {
    return m_texture.get();
}

const BoundingBox& Marker::bounds() const {
    return m_bounds;
}

const glm::dvec2& Marker::origin() const {
    return m_origin;
}

const glm::mat4& Marker::modelMatrix() const {
    return m_modelMatrix;
}

const glm::mat4& Marker::modelViewProjectionMatrix() const {
    return m_modelViewProjectionMatrix;
}

bool Marker::isEasing() const {
    return !m_ease.finished();
}

bool Marker::isVisible() const {
    return m_visible;
}

uint32_t Marker::selectionColor() const {
    return m_selectionColor;
}

bool Marker::compareByDrawOrder(const std::unique_ptr<Marker>& lhs, const std::unique_ptr<Marker>& rhs) {
    return lhs->m_drawOrder < rhs->m_drawOrder;
}

} // namespace Tangram
