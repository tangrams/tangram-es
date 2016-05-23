#include "pointStyleBuilder.h"
#include "scene/drawRule.h"
#include "scene/stops.h"
#include "scene/spriteAtlas.h"
#include "labels/labelCollider.h"
#include "labels/spriteLabel.h"
#include "util/geom.h"
#include "data/propertyItem.h" // Include wherever Properties is used!

#include "tile/tile.h"

namespace Tangram {


void IconMesh::setTextLabels(std::unique_ptr<StyledMesh> _textLabels) {

    auto* mesh = static_cast<TextLabels*>(_textLabels.get());
    auto& labels = mesh->getLabels();

    typedef std::vector<std::unique_ptr<Label>>::iterator iter_t;
    m_labels.insert(m_labels.end(),
                    std::move_iterator<iter_t>(labels.begin()),
                    std::move_iterator<iter_t>(labels.end()));

    labels.clear();

    textLabels = std::move(_textLabels);
}

void PointStyleBuilder::addLayoutItems(LabelCollider& _layout) {
    _layout.addLabels(m_labels);
    m_textStyleBuilder->addLayoutItems(_layout);
}

std::unique_ptr<StyledMesh> PointStyleBuilder::build() {
    if (m_quads.empty()) { return nullptr; }

    m_spriteLabels->setQuads(m_quads);

    m_quads.clear();

    m_iconMesh->setLabels(m_labels);
    m_iconMesh->spriteLabels = std::move(m_spriteLabels);

    if (auto textLabels = m_textStyleBuilder->build()) {
        m_iconMesh->setTextLabels(std::move(textLabels));
    }

    return std::move(m_iconMesh);
}

void PointStyleBuilder::setup(const Tile& _tile) {
    m_zoom = _tile.getID().z;
    m_spriteLabels = std::make_unique<SpriteLabels>(m_style);

    m_textStyleBuilder->setup(_tile);
    m_iconMesh = std::make_unique<IconMesh>();
}

bool PointStyleBuilder::checkRule(const DrawRule& _rule) const {
    uint32_t checkColor;
    // require a color or texture atlas/texture to be valid
    if (!_rule.get(StyleParamKey::color, checkColor) &&
        !m_style.texture() &&
        !m_style.spriteAtlas()) {
        return false;
    }
    return true;
}

auto PointStyleBuilder::applyRule(const DrawRule& _rule, const Properties& _props) const -> PointStyle::Parameters {

    PointStyle::Parameters p;
    glm::vec2 size;
    std::string anchor;

    _rule.get(StyleParamKey::color, p.color);
    _rule.get(StyleParamKey::sprite, p.sprite);
    _rule.get(StyleParamKey::offset, p.labelOptions.offset);

    uint32_t priority;
    if (_rule.get(StyleParamKey::priority, priority)) {
        p.labelOptions.priority = (float)priority;
    }

    _rule.get(StyleParamKey::sprite_default, p.spriteDefault);
    _rule.get(StyleParamKey::centroid, p.centroid);
    _rule.get(StyleParamKey::interactive, p.labelOptions.interactive);
    _rule.get(StyleParamKey::collide, p.labelOptions.collide);
    _rule.get(StyleParamKey::transition_hide_time, p.labelOptions.hideTransition.time);
    _rule.get(StyleParamKey::transition_selected_time, p.labelOptions.selectTransition.time);
    _rule.get(StyleParamKey::transition_show_time, p.labelOptions.showTransition.time);
    _rule.get(StyleParamKey::anchor, anchor);

    auto sizeParam = _rule.findParameter(StyleParamKey::size);
    if (sizeParam.stops && sizeParam.value.is<float>()) {
        float lowerSize = sizeParam.value.get<float>();
        float higherSize = sizeParam.stops->evalWidth(m_zoom + 1);
        p.extrudeScale = (higherSize - lowerSize) * 0.5f - 1.f;
        p.size = glm::vec2(lowerSize);
    } else if (_rule.get(StyleParamKey::size, size)) {
        if (size.x == 0.f || std::isnan(size.y)) {
            p.size = glm::vec2(size.x);
        } else {
            p.size = size;
        }
    } else {
        p.size = glm::vec2(NAN, NAN);
    }

    LabelProperty::anchor(anchor, p.anchor);

    if (p.labelOptions.interactive) {
        p.labelOptions.properties = std::make_shared<Properties>(_props);
    }

    std::hash<PointStyle::Parameters> hash;
    p.labelOptions.paramHash = hash(p);

    return p;
}

void PointStyleBuilder::addLabel(const Point& _point, const glm::vec4& _quad,
                                 const PointStyle::Parameters& _params) {

    m_labels.push_back(std::make_unique<SpriteLabel>(Label::Transform{glm::vec2(_point)},
                                                     _params.size,
                                                     _params.labelOptions,
                                                     _params.extrudeScale,
                                                     _params.anchor,
                                                     *m_spriteLabels,
                                                     m_quads.size()));

    glm::i16vec2 size = _params.size * SpriteVertex::position_scale;

    // Attribute will be normalized - scale to max short;
    glm::vec2 uvTR = glm::vec2{_quad.z, _quad.w} * SpriteVertex::texture_scale;
    glm::vec2 uvBL = glm::vec2{_quad.x, _quad.y} * SpriteVertex::texture_scale;

    int16_t extrude = _params.extrudeScale * SpriteVertex::extrusion_scale;

    m_quads.push_back({
            {{{0, 0},
             {uvBL.x, uvTR.y},
             {-extrude, extrude}},
            {{size.x, 0},
             {uvTR.x, uvTR.y},
             {extrude, extrude}},
            {{0, -size.y},
             {uvBL.x, uvBL.y},
             {-extrude, -extrude}},
            {{size.x, -size.y},
             {uvTR.x, uvBL.y},
             {extrude, -extrude}}},
            _params.color});
}

bool PointStyleBuilder::getUVQuad(PointStyle::Parameters& _params, glm::vec4& _quad) const {
    _quad = glm::vec4(0.0, 0.0, 1.0, 1.0);

    if (m_style.spriteAtlas()) {
        SpriteNode spriteNode;

        if (!m_style.spriteAtlas()->getSpriteNode(_params.sprite, spriteNode) &&
            !m_style.spriteAtlas()->getSpriteNode(_params.spriteDefault, spriteNode)) {
            return false;
        }

        if (std::isnan(_params.size.x)) {
            _params.size = spriteNode.m_size;
        }

        _quad.x = spriteNode.m_uvBL.x;
        _quad.y = spriteNode.m_uvBL.y;
        _quad.z = spriteNode.m_uvTR.x;
        _quad.w = spriteNode.m_uvTR.y;
    } else {
        // default point size
        if (std::isnan(_params.size.x)) {
            _params.size = glm::vec2(8.0);
        }
    }

    _params.size *= m_style.pixelScale();

    return true;
}

void PointStyleBuilder::addPoint(const Point& _point, const Properties& _props,
                                 const DrawRule& _rule) {

    PointStyle::Parameters p = applyRule(_rule, _props);
    glm::vec4 uvsQuad;

    if (!getUVQuad(p, uvsQuad)) {
        return;
    }

    addLabel(_point, uvsQuad, p);
}

void PointStyleBuilder::addLine(const Line& _line, const Properties& _props,
                                const DrawRule& _rule) {

    PointStyle::Parameters p = applyRule(_rule, _props);
    glm::vec4 uvsQuad;

    if (!getUVQuad(p, uvsQuad)) {
        return;
    }

    for (size_t i = 0; i < _line.size(); ++i) {
        addLabel(_line[i], uvsQuad, p);
    }
}

void PointStyleBuilder::addPolygon(const Polygon& _polygon, const Properties& _props,
                                   const DrawRule& _rule) {

    PointStyle::Parameters p = applyRule(_rule, _props);
    glm::vec4 uvsQuad;

    if (!getUVQuad(p, uvsQuad)) {
        return;
    }

    if (!p.centroid) {
        for (auto line : _polygon) {
            for (auto point : line) {
                addLabel(point, uvsQuad, p);
            }
        }
    } else {
        glm::vec2 c = centroid(_polygon);

        addLabel(Point{c,0}, uvsQuad, p);
    }
}

void PointStyleBuilder::addFeature(const Feature& _feat, const DrawRule& _rule) {

    size_t iconsStart = m_labels.size();

    StyleBuilder::addFeature(_feat, _rule);

    size_t iconsCount = m_labels.size() - iconsStart;

    if (_rule.contains(StyleParamKey::point_text)) {
        if (iconsCount == 0) { return; }

        auto& textStyleBuilder = static_cast<TextStyleBuilder&>(*m_textStyleBuilder);
        auto& textLabels = *textStyleBuilder.labels();

        size_t textStart = textLabels.size();

        textStyleBuilder.addFeatureCommon(_feat, _rule, true);

        size_t textCount = textLabels.size() - textStart;

        if (iconsCount == textCount) {
            bool definePriority = !_rule.contains(StyleParamKey::text_priority);

            for (size_t i = 0; i < textCount; ++i) {
                auto& tLabel = textLabels[textStart + i];
                auto& pLabel = m_labels[iconsStart + i];

                // Link labels together
                tLabel->setParent(*pLabel, definePriority);
            }
        }
    }
}

}
