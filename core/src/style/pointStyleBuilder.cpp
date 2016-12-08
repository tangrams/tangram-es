#include "pointStyleBuilder.h"

#include "data/propertyItem.h" // Include wherever Properties is used!
#include "labels/labelCollider.h"
#include "labels/spriteLabel.h"
#include "marker/marker.h"
#include "scene/drawRule.h"
#include "scene/spriteAtlas.h"
#include "scene/stops.h"
#include "tangram.h"
#include "tile/tile.h"
#include "util/geom.h"
#include "util/featureSelection.h"

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


    if (Tangram::getDebugFlag(DebugFlags::draw_all_labels)) {

        m_iconMesh->setLabels(m_labels);

    } else {
        size_t sumLabels = 0;

        // Determine number of labels
       for (auto& label : m_labels) {
           if (label->state() != Label::State::dead) { sumLabels +=1; }
        }

       std::vector<std::unique_ptr<Label>> labels;
       labels.reserve(sumLabels);

       for (auto& label : m_labels) {
           if (label->state() != Label::State::dead) {
               labels.push_back(std::move(label));
           }
       }
       m_iconMesh->setLabels(labels);
    }

    std::vector<SpriteQuad> quads(m_quads);
    m_spriteLabels->setQuads(std::move(quads));

    m_quads.clear();
    m_labels.clear();

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

void PointStyleBuilder::setup(const Marker& _marker, int zoom) {
    m_zoom = zoom;
    m_spriteLabels = std::make_unique<SpriteLabels>(m_style);

    m_textStyleBuilder->setup(_marker, zoom);
    m_iconMesh = std::make_unique<IconMesh>();

    m_texture = _marker.texture();
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

    _rule.get(StyleParamKey::color, p.color);
    _rule.get(StyleParamKey::sprite, p.sprite);
    _rule.get(StyleParamKey::offset, p.labelOptions.offset);

    uint32_t priority;
    if (_rule.get(StyleParamKey::priority, priority)) {
        p.labelOptions.priority = (float)priority;
    }

    _rule.get(StyleParamKey::sprite_default, p.spriteDefault);
    _rule.get(StyleParamKey::centroid, p.centroid);
    _rule.get(StyleParamKey::interactive, p.interactive);
    _rule.get(StyleParamKey::collide, p.labelOptions.collide);
    _rule.get(StyleParamKey::transition_hide_time, p.labelOptions.hideTransition.time);
    _rule.get(StyleParamKey::transition_selected_time, p.labelOptions.selectTransition.time);
    _rule.get(StyleParamKey::transition_show_time, p.labelOptions.showTransition.time);
    _rule.get(StyleParamKey::flat, p.labelOptions.flat);
    _rule.get(StyleParamKey::anchor, p.labelOptions.anchors);

    if (p.labelOptions.anchors.count == 0) {
        p.labelOptions.anchors.anchor = { {LabelProperty::Anchor::center} };
        p.labelOptions.anchors.count = 1;
    }

    _rule.get(StyleParamKey::angle, p.labelOptions.angle);

    auto sizeParam = _rule.findParameter(StyleParamKey::size);
    if (sizeParam.stops) {
        if (sizeParam.value.is<float>()) {
            float lowerSize = sizeParam.value.get<float>();
            float higherSize = sizeParam.stops->evalExpFloat(m_zoom + 1);
            p.extrudeScale = (higherSize - lowerSize) * 0.5f - 1.f;
            p.size = glm::vec2(lowerSize);
        } else if (sizeParam.value.is<glm::vec2>()) {
            p.size = sizeParam.stops->evalExpVec2(m_zoom + 1);
        } else {
            p.size = glm::vec2(NAN, NAN);
        }
    } else if (_rule.get(StyleParamKey::size, size)) {
        if (size.x == 0.f || std::isnan(size.y)) {
            p.size = glm::vec2(size.x);
        } else {
            p.size = size;
        }
    } else {
        p.size = glm::vec2(NAN, NAN);
    }

    std::hash<PointStyle::Parameters> hash;
    p.labelOptions.paramHash = hash(p);

    if (p.interactive) {
        p.labelOptions.featureId = _rule.selectionColor;
    }

    return p;
}

void PointStyleBuilder::addLabel(const Point& _point, const glm::vec4& _quad,
                                 const PointStyle::Parameters& _params, const DrawRule& _rule) {

    uint32_t selectionColor = 0;

    if (_params.interactive) {
        if (_rule.featureSelection) {
            selectionColor = _rule.featureSelection->nextColorIdentifier();
        } else {
            selectionColor = _rule.selectionColor;
        }
    }

    m_labels.push_back(std::make_unique<SpriteLabel>(glm::vec3(glm::vec2(_point), m_zoom),
                                                     _params.size,
                                                     _params.labelOptions,
                                                     SpriteLabel::VertexAttributes{_params.color,
                                                             selectionColor, _params.extrudeScale },
                                                     m_texture,
                                                     *m_spriteLabels,
                                                     m_quads.size()));

    glm::i16vec2 size = _params.size;

    // Attribute will be normalized - scale to max short;
    glm::vec2 uvTR = glm::vec2{_quad.z, _quad.w} * SpriteVertex::texture_scale;
    glm::vec2 uvBL = glm::vec2{_quad.x, _quad.y} * SpriteVertex::texture_scale;

    float sx = size.x * 0.5f;
    float sy = size.y * 0.5f;

    glm::vec2 v0(-sx, sy);
    glm::vec2 v1(sx, sy);
    glm::vec2 v2(-sx, -sy);
    glm::vec2 v3(sx, -sy);

    if (_params.labelOptions.angle != 0.f) {
        // Rotate the sprite icon quad vertices in clockwise order
        glm::vec2 rotation(cos(-DEG_TO_RAD * _params.labelOptions.angle),
                           sin(-DEG_TO_RAD * _params.labelOptions.angle));

        v0 = rotateBy(v0, rotation);
        v1 = rotateBy(v1, rotation);
        v2 = rotateBy(v2, rotation);
        v3 = rotateBy(v3, rotation);
    }

    m_quads.push_back({{
        {v0, {uvBL.x, uvTR.y}},
        {v1, {uvTR.x, uvTR.y}},
        {v2, {uvBL.x, uvBL.y}},
        {v3, {uvTR.x, uvBL.y}}}
        });
}

bool PointStyleBuilder::getUVQuad(PointStyle::Parameters& _params, glm::vec4& _quad) const {
    _quad = glm::vec4(0.0, 1.0, 1.0, 0.0);

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

bool PointStyleBuilder::addPoint(const Point& _point, const Properties& _props,
                                 const DrawRule& _rule) {

    PointStyle::Parameters p = applyRule(_rule, _props);
    glm::vec4 uvsQuad;

    if (!getUVQuad(p, uvsQuad)) {
        return false;
    }

    addLabel(_point, uvsQuad, p, _rule);

    return true;
}

bool PointStyleBuilder::addLine(const Line& _line, const Properties& _props,
                                const DrawRule& _rule) {

    PointStyle::Parameters p = applyRule(_rule, _props);
    glm::vec4 uvsQuad;

    if (!getUVQuad(p, uvsQuad)) {
        return false;
    }

    for (size_t i = 0; i < _line.size(); ++i) {
        addLabel(_line[i], uvsQuad, p, _rule);
    }

    return true;
}

bool PointStyleBuilder::addPolygon(const Polygon& _polygon, const Properties& _props,
                                   const DrawRule& _rule) {

    PointStyle::Parameters p = applyRule(_rule, _props);
    glm::vec4 uvsQuad;

    if (!getUVQuad(p, uvsQuad)) {
        return false;
    }

    if (!p.centroid) {
        for (auto line : _polygon) {
            for (auto point : line) {
                addLabel(point, uvsQuad, p, _rule);
            }
        }
    } else {
        glm::vec2 c = centroid(_polygon);

        addLabel(Point{c,0}, uvsQuad, p, _rule);
    }

    return true;
}

bool PointStyleBuilder::addFeature(const Feature& _feat, const DrawRule& _rule) {

    size_t iconsStart = m_labels.size();

    if (!StyleBuilder::addFeature(_feat, _rule)) {
        return false;
    }

    size_t iconsCount = m_labels.size() - iconsStart;

    bool textVisible = true;
    _rule.get(StyleParamKey::text_visible, textVisible);

    if (textVisible && _rule.contains(StyleParamKey::point_text)) {
        if (iconsCount == 0) { return true; }

        auto& textStyleBuilder = static_cast<TextStyleBuilder&>(*m_textStyleBuilder);
        auto& textLabels = *textStyleBuilder.labels();

        size_t textStart = textLabels.size();

        if (!textStyleBuilder.addFeatureCommon(_feat, _rule, true)) { return true; }

        size_t textCount = textLabels.size() - textStart;

        if (iconsCount == textCount) {
            bool definePriority = !_rule.contains(StyleParamKey::text_priority);
            bool defineCollide = _rule.contains(StyleParamKey::collide);

            for (size_t i = 0; i < textCount; ++i) {
                auto& tLabel = textLabels[textStart + i];
                auto& pLabel = m_labels[iconsStart + i];

                // Link labels together
                tLabel->setParent(*pLabel, definePriority, defineCollide);
            }
        }
    }
    return true;
}

}
