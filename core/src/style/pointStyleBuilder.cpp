#include "style/pointStyleBuilder.h"

#include "data/propertyItem.h"
#include "labels/labelCollider.h"
#include "labels/spriteLabel.h"
#include "log.h"
#include "marker/marker.h"
#include "map.h"
#include "scene/drawRule.h"
#include "scene/spriteAtlas.h"
#include "scene/stops.h"
#include "selection/featureSelection.h"
#include "tile/tile.h"
#include "util/geom.h"
#include "util/lineSampler.h"
#include "view/view.h"

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
    m_styleZoom = _tile.getID().s;

    // < 1.0 when overzooming a tile
    m_tileScale = pow(2, _tile.getID().s - _tile.getID().z);

    m_spriteLabels = std::make_unique<SpriteLabels>(m_style);

    m_textStyleBuilder->setup(_tile);
    m_iconMesh = std::make_unique<IconMesh>();
}

void PointStyleBuilder::setup(const Marker& _marker, int zoom) {
    m_zoom = zoom;
    m_styleZoom = zoom;
    m_spriteLabels = std::make_unique<SpriteLabels>(m_style);

    m_textStyleBuilder->setup(_marker, zoom);
    m_iconMesh = std::make_unique<IconMesh>();

    m_texture = _marker.texture();
}

bool PointStyleBuilder::checkRule(const DrawRule& _rule) const {
    // require a color or texture atlas/texture to be valid
    uint32_t color;
    if (_rule.get(StyleParamKey::color, color)) {
        return true;
    }

    std::string texture;
    if (_rule.get(StyleParamKey::texture, texture) && !texture.empty()) {
        return true;
    }

    if (m_style.defaultTexture()) {
        return true;
    }

    return false;
}

auto PointStyleBuilder::applyRule(const DrawRule& _rule) const -> Parameters {

    Parameters p;

    _rule.get(StyleParamKey::color, p.color);
    _rule.get(StyleParamKey::sprite, p.sprite);
    _rule.get(StyleParamKey::offset, p.labelOptions.offset);
    _rule.get(StyleParamKey::buffer, p.labelOptions.buffer);

    uint32_t priority = 0;
    std::string repeatGroup;
    StyleParam::Width repeatDistance;

    if (_rule.get(StyleParamKey::priority, priority)) {
        p.labelOptions.priority = (float)priority;
    }

    _rule.get(StyleParamKey::sprite_default, p.spriteDefault);
    p.dynamicTexture = _rule.get(StyleParamKey::texture, p.texture);

    _rule.get(StyleParamKey::placement, p.placement);
    StyleParam::Width placementSpacing;
    auto placementSpacingParam = _rule.findParameter(StyleParamKey::placement_spacing);
    if (placementSpacingParam.stops) {
        p.placementSpacing = placementSpacingParam.stops->evalFloat(m_styleZoom);
    } else if (_rule.get(StyleParamKey::placement_spacing, placementSpacing)) {
        p.placementSpacing = placementSpacing.value;
    }
    auto placementMinLengthParam = _rule.findParameter(StyleParamKey::placement_min_length_ratio);
    if (placementMinLengthParam.stops) {
        p.placementMinLengthRatio = placementMinLengthParam.stops->evalFloat(m_styleZoom);
    } else {
        _rule.get(StyleParamKey::placement_min_length_ratio, p.placementMinLengthRatio);
    }
    _rule.get(StyleParamKey::tile_edges, p.keepTileEdges);
    _rule.get(StyleParamKey::interactive, p.interactive);
    _rule.get(StyleParamKey::collide, p.labelOptions.collide);
    _rule.get(StyleParamKey::transition_hide_time, p.labelOptions.hideTransition.time);
    _rule.get(StyleParamKey::transition_selected_time, p.labelOptions.selectTransition.time);
    _rule.get(StyleParamKey::transition_show_time, p.labelOptions.showTransition.time);
    _rule.get(StyleParamKey::flat, p.labelOptions.flat);
    _rule.get(StyleParamKey::anchor, p.labelOptions.anchors);

    if (_rule.get(StyleParamKey::repeat_distance, repeatDistance)) {
        p.labelOptions.repeatDistance = repeatDistance.value;
    }

    if (p.labelOptions.repeatDistance > 0.f) {
        size_t repeatGroupHash = 0;
        if (_rule.get(StyleParamKey::repeat_group, repeatGroup)) {
            hash_combine(repeatGroupHash, repeatGroup);
        } else {
            repeatGroupHash = _rule.getParamSetHash();
        }

        p.labelOptions.repeatGroup = repeatGroupHash;
        p.labelOptions.repeatDistance *= m_style.pixelScale();
    }

    if (p.labelOptions.anchors.count == 0) {
        p.labelOptions.anchors.anchor = { {LabelProperty::Anchor::center} };
        p.labelOptions.anchors.count = 1;
    }

    _rule.get(StyleParamKey::angle, p.labelOptions.angle);
    if (std::isnan(p.labelOptions.angle)) {
        p.autoAngle = true;
    }

    auto& strokeWidth = _rule.findParameter(StyleParamKey::outline_width);

    if (_rule.get(StyleParamKey::outline_color, p.outlineColor) &&
        strokeWidth.value.is<StyleParam::Width>()) {

        auto& widthParam = strokeWidth.value.get<StyleParam::Width>();

        p.outlineWidth = widthParam.value * m_style.pixelScale();
    }


    std::hash<Parameters> hash;
    p.labelOptions.paramHash = hash(p);

    if (p.interactive) {
        p.labelOptions.featureId = _rule.selectionColor;
    }

    return p;
}

void PointStyleBuilder::addLabel(const Point& _point, const glm::vec4& _quad, Texture* _texture,
                                 const Parameters& _params, const DrawRule& _rule) {

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
                                                     SpriteLabel::VertexAttributes{
                                                             _params.color,
                                                             selectionColor,
                                                             _params.outlineColor,
                                                             _params.antialiasFactor,
                                                             _params.extrudeScale },
                                                     _texture,
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

bool PointStyleBuilder::getTexture(const Parameters& _params, Texture **_texture) const {

    auto texture = m_style.defaultTexture().get();

    if (m_texture) {
        // If marker texture has been assigned, always use it.
        texture = m_texture;

    } else if (_params.dynamicTexture) {

        if (_params.texture == "") {
            texture = nullptr;
        } else {
            if (!m_style.textures()) {
                return false;
            }

            auto it = m_style.textures()->find(_params.texture);
            if (it == m_style.textures()->end()) {
                return false;
            }
            texture = it->second.get();
        }
    }

    if (texture) {
        *_texture = texture;
    }

    return true;
}

bool PointStyleBuilder::evalSizeParam(const DrawRule& _rule, Parameters& _params, const Texture* _texture) const {
    StyleParam::SizeValue size;
    SpriteNode spriteNode;
    glm::vec2 spriteSize(NAN);

    if (_texture) {
        spriteSize = glm::vec2{_texture->getWidth(), _texture->getHeight()} * _texture->invDensity();

        const auto &atlas = _texture->spriteAtlas();
        if (atlas) {
            if (!atlas->getSpriteNode(_params.sprite, spriteNode) &&
                !atlas->getSpriteNode(_params.spriteDefault, spriteNode)) {
                return false;
            }
            spriteSize = spriteNode.m_size * _texture->invDensity();
        } else if ( !_params.sprite.empty() || !_params.spriteDefault.empty()) {
            // missing sprite atlas for texture but sprite specified in draw rule
            return false;
        }
    }

    auto sizeParam = _rule.findParameter(StyleParamKey::size);

    if (sizeParam.stops) {
        glm::vec2 lower = sizeParam.stops->evalSize(m_styleZoom, spriteSize);
        glm::vec2 higher = sizeParam.stops->evalSize(m_styleZoom + 1, spriteSize);
        if ((std::isnan(lower.x) || std::isnan(lower.y)) || (std::isnan(higher.x) || std::isnan(higher.y))) {
            // Illegal size values were evaluated.
            return false;
        }
        _params.size = lower;
        _params.extrudeScale = higher.x - lower.x;

    } else if (_rule.get(StyleParamKey::size, size)) {
        glm::vec2 pixelSize = size.getSizePixels(spriteSize);
        if (std::isnan(pixelSize.x) || std::isnan(pixelSize.y)) {
            // Illegal size values were evaluated.
            return false;
        }
        _params.size = pixelSize;
    } else {
        _params.size = spriteSize;
    }

    _params.size *= m_style.pixelScale();

    return true;
}

bool PointStyleBuilder::getUVQuad(Parameters& _params, glm::vec4& _quad, const Texture* _texture) const {

    _quad = glm::vec4(0.0, 1.0, 1.0, 0.0);

    if (_texture) {
        const auto& atlas = _texture->spriteAtlas();
        if (atlas) {
            SpriteNode spriteNode;
            if (!atlas->getSpriteNode(_params.sprite, spriteNode) &&
                !atlas->getSpriteNode(_params.spriteDefault, spriteNode)) {
                return false;
            }
            _quad.x = spriteNode.m_uvBL.x;
            _quad.y = spriteNode.m_uvBL.y;
            _quad.z = spriteNode.m_uvTR.x;
            _quad.w = spriteNode.m_uvTR.y;
        }
    } else {

        float fillEdge = _params.size.x;
        float outlineEdge = 1.f;

        // Adds half the outline width in each direction (* 0.5 * 2.0)
        _params.size += _params.outlineWidth;

        // Fraction at which fill ends
        fillEdge /= _params.size.x;

        if (_params.outlineWidth > 0.f) {
            // Fraction at which outline starts
            outlineEdge = fillEdge - _params.outlineWidth / _params.size.x;
        }

        fillEdge = std::max(0.0001f, fillEdge);
        outlineEdge = std::max(0.0001f, outlineEdge);

        _quad = glm::vec4(-outlineEdge, -fillEdge,
                          outlineEdge, fillEdge);

        _params.antialiasFactor = std::numeric_limits<int16_t>::max() / _params.size.x * 2.0f;
    }

    return true;
}

void PointStyleBuilder::labelPointsPlacing(const Line& _line, const glm::vec4& _uvsQuad, Texture* _texture,
                                           Parameters& params, const DrawRule& _rule) {

    if (_line.size() < 2) { return; }

    auto isOutsideTile = [](const Point& p) {
        float tolerance = 0.0005;
        float tile_min = 0.0 + tolerance;
        float tile_max = 1.0 - tolerance;
        return ((p.x < tile_min) || (p.x > tile_max) ||
                (p.y < tile_min) || (p.y > tile_max));
    };

    auto angleBetween = [](const Point& p, const Point& q) {
        return RAD_TO_DEG * atan2(q[0] - p[0], q[1] - p[1]);
    };

    float minLineLength = std::max(params.size.x, params.size.y) *
        params.placementMinLengthRatio * m_style.pixelScale() /
        (MapProjection::tileSize() * m_tileScale);

    switch(params.placement) {
        case LabelProperty::Placement::vertex: {
            for (size_t i = 0; i < _line.size() - 1; i++) {
                auto& p = _line[i];
                auto& q = _line[i+1];
                if (params.keepTileEdges || !isOutsideTile(p)) {
                    if (params.autoAngle) {
                        params.labelOptions.angle = angleBetween(p, q);
                    }
                    addLabel(p, _uvsQuad, _texture, params, _rule);
                    if (i == _line.size() - 2) {
                        // Place label on endpoint
                        addLabel(q, _uvsQuad, _texture, params, _rule);
                    }
                }
            }
            break;
        }
        case LabelProperty::Placement::midpoint:
            for (size_t i = 0; i < _line.size() - 1; i++) {
                auto& p = _line[i];
                auto& q = _line[i+1];
                if ( (params.keepTileEdges || !isOutsideTile(p)) &&
                     (minLineLength == 0.0f || glm::distance(p, q) > minLineLength) ) {
                    if (params.autoAngle) {
                        params.labelOptions.angle = angleBetween(p, q);
                    }
                    glm::vec3 midpoint(0.5f * (p.x + q.x), 0.5f * (p.y + q.y), 0.0f);
                    addLabel(midpoint, _uvsQuad, _texture, params, _rule);
                }
            }
            break;
        case LabelProperty::Placement::spaced: {
            LineSampler<std::vector<glm::vec3>> sampler;

            sampler.set(_line);

            float lineLength = sampler.sumLength();
            if (lineLength <= minLineLength) { break; }

            float spacing = params.placementSpacing * m_style.pixelScale() /
                (MapProjection::tileSize() * m_tileScale);

            int numLabels = std::max(std::floor(lineLength / spacing), 1.0f);
            float remainderLength = lineLength - (numLabels - 1) * spacing;
            float distance = 0.5 * remainderLength;
            glm::vec2 p, r;
            sampler.advance(distance, p, r);
            do {

                if (sampler.lengthToPrevSegment() < minLineLength*0.5 ||
                    sampler.lengthToNextSegment() < minLineLength*0.5) {
                    continue;
                }
                if (params.autoAngle) {
                    params.labelOptions.angle = RAD_TO_DEG * atan2(r.x, r.y);
                }

                addLabel({p.x, p.y}, _uvsQuad, _texture, params, _rule);

            } while (sampler.advance(spacing, p, r));
        }
        break;
        case LabelProperty::Placement::centroid:
            // nothing to be done here.
            break;
    }
}

bool PointStyleBuilder::addPoint(const Point& _point, const Properties& _props,
                                 const DrawRule& _rule) {

    Parameters p = applyRule(_rule);
    glm::vec4 uvsQuad;
    Texture* texture = nullptr;

    if (!getTexture(p, &texture)) {
        return false;
    }
    if (!evalSizeParam(_rule, p, texture)) {
        return false;
    }
    if (!getUVQuad(p, uvsQuad, texture)) {
        return false;
    }

    addLabel(_point, uvsQuad, texture, p, _rule);

    return true;
}

bool PointStyleBuilder::addLine(const Line& _line, const Properties& _props,
                                const DrawRule& _rule) {

    Parameters p = applyRule(_rule);
    glm::vec4 uvsQuad;
    Texture* texture = nullptr;

    if (!getTexture(p, &texture)) {
        return false;
    }
    if (!evalSizeParam(_rule, p, texture)) {
        return false;
    }
    if (!getUVQuad(p, uvsQuad, texture)) {
        return false;
    }

    labelPointsPlacing(_line, uvsQuad, texture, p, _rule);

    return true;
}

bool PointStyleBuilder::addPolygon(const Polygon& _polygon, const Properties& _props,
                                   const DrawRule& _rule) {

    Parameters p = applyRule(_rule);
    glm::vec4 uvsQuad;
    Texture* texture = nullptr;

    if (!getTexture(p, &texture)) {
        return false;
    }
    if (!evalSizeParam(_rule, p, texture)) {
        return false;
    }
    if (!getUVQuad(p, uvsQuad, texture)) {
        return false;
    }

    if (p.placement != LabelProperty::centroid) {
        for (auto line : _polygon) {
            labelPointsPlacing(line, uvsQuad, texture, p, _rule);
        }
    } else {
        if (!_polygon.empty()) {
            glm::vec2 c;
            c = centroid(_polygon.front().begin(), _polygon.front().end());
            addLabel(c, uvsQuad, texture, p, _rule);
        }
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

        TextStyle::Parameters params = textStyleBuilder.applyRule(_rule, _feat.props, true);

        TextStyleBuilder::LabelAttributes attrib;
        if (textStyleBuilder.prepareLabel(params, Label::Type::point, attrib)) {

            for (size_t i = 0; i < iconsCount; i++) {
                auto pLabel = static_cast<SpriteLabel*>(m_labels[iconsStart + i].get());
                auto p = pLabel->modelCenter();
                textStyleBuilder.addLabel(Label::Type::point, {{p, p}}, params, attrib, _rule);

                bool definePriority = !_rule.contains(StyleParamKey::text_priority);
                bool defineCollide = _rule.contains(StyleParamKey::collide);

                // Link labels together
                textLabels.back()->setRelative(*pLabel, definePriority, defineCollide);
            }
        }
    }

    return true;
}

}
