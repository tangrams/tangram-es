#include "textStyleBuilder.h"

#include "labels/labelCollider.h"
#include "labels/labelSet.h"
#include "labels/textLabel.h"
#include "labels/textLabels.h"

#include "data/propertyItem.h" // Include wherever Properties is used!
#include "scene/drawRule.h"
#include "tile/tile.h"
#include "util/geom.h"
#include "util/mapProjection.h"
#include "view/view.h"
#include "tangram.h"

#include <cmath>
#include <locale>
#include <mutex>
#include <algorithm>

namespace Tangram {

const static std::string key_name("name");

TextStyleBuilder::TextStyleBuilder(const TextStyle& _style)
    : StyleBuilder(_style),
      m_style(_style) {}

void TextStyleBuilder::setup(const Tile& _tile){
    m_tileSize = _tile.getProjection()->TileSize();
    m_tileSize *= m_style.pixelScale();

    float tileScale = pow(2, _tile.getID().s - _tile.getID().z);
    m_tileSize *= tileScale;

    // add scale factor to the next zoom-level
    m_tileSize *= 2;

    m_atlasRefs.reset();

    m_textLabels = std::make_unique<TextLabels>(m_style);
}

void TextStyleBuilder::addLayoutItems(LabelCollider& _layout) {
    _layout.addLabels(m_labels);
}

std::unique_ptr<StyledMesh> TextStyleBuilder::build() {

    if (m_quads.empty()) { return nullptr; }

    if (Tangram::getDebugFlag(DebugFlags::draw_all_labels)) {
        m_textLabels->setLabels(m_labels);

        std::vector<GlyphQuad> quads(m_quads);
        m_textLabels->setQuads(std::move(quads), m_atlasRefs);

    } else {

        // TODO this could probably done more elegant

        int quadPos = 0;
        size_t sumQuads = 0;
        size_t sumLabels = 0;
        bool added = false;

        // Determine number of labels and size of final quads vector
        for (auto& label : m_labels) {
            auto* textLabel = static_cast<TextLabel*>(label.get());

            auto& ranges = textLabel->textRanges();

            bool active = textLabel->state() != Label::State::dead;

            if (ranges.back().end() != quadPos) {
                quadPos = ranges.back().end();
                added = false;
            }

            if (!active) { continue; }

            sumLabels +=1;
            if (!added) {
                added = true;

                for (auto& textRange : ranges) {
                    sumQuads += textRange.length;
                }
            }
        }

        size_t quadEnd = 0;
        size_t quadStart = 0;
        quadPos = 0;

        std::vector<std::unique_ptr<Label>> labels;
        labels.reserve(sumLabels);

        std::vector<GlyphQuad> quads;
        quads.reserve(sumQuads);

        // Add only alive labels
        for (auto& label : m_labels) {
            auto* textLabel = static_cast<TextLabel*>(label.get());

            bool active = textLabel->state() != Label::State::dead;
            if (!active) { continue; }

            auto& ranges = textLabel->textRanges();

            // Add the quads of line-labels only once
            if (ranges.back().end() != quadPos) {
                quadStart = quadEnd;
                quadPos = ranges.back().end();

                for (auto& textRange : ranges) {
                    if (textRange.length > 0) {
                        quadEnd += textRange.length;

                        auto it = m_quads.begin() + textRange.start;
                        quads.insert(quads.end(), it, it + textRange.length);
                    }
                }
            }

            // Update TextRange
            auto start = quadStart;

            for (auto& textRange : ranges) {
                textRange.start = start;
                start += textRange.length;
            }

            labels.push_back(std::move(label));
        }

        m_textLabels->setLabels(labels);
        m_textLabels->setQuads(std::move(quads), m_atlasRefs);
    }

    m_labels.clear();
    m_quads.clear();

    return std::move(m_textLabels);
}

bool TextStyleBuilder::addFeatureCommon(const Feature& _feat, const DrawRule& _rule, bool _iconText) {
    TextStyle::Parameters params = applyRule(_rule, _feat.props, _iconText);

    Label::Type labelType;
    if (_feat.geometryType == GeometryType::lines) {
        labelType = Label::Type::line;
        params.wordWrap = false;
    } else {
        labelType = Label::Type::point;
    }

    // Keep start position of new quads
    size_t quadsStart = m_quads.size();
    size_t numLabels = m_labels.size();

    if (!prepareLabel(params, labelType)) { return false; }

    if (_feat.geometryType == GeometryType::points) {
        for (auto& point : _feat.points) {
            auto p = glm::vec2(point);
            addLabel(params, Label::Type::point, { p, p });
        }

    } else if (_feat.geometryType == GeometryType::polygons) {
        for (auto& polygon : _feat.polygons) {
            if (_iconText) {
                auto p = centroid(polygon);
                addLabel(params, Label::Type::point, { p, p });
            } else {
                for (auto& line : polygon) {
                    for (auto& point : line) {
                        auto p = glm::vec2(point);
                        addLabel(params, Label::Type::point, { p });
                    }
                }
            }
        }

    } else if (_feat.geometryType == GeometryType::lines) {

        if (_iconText) {
            for (auto& line : _feat.lines) {
                for (auto& point : line) {
                    auto p = glm::vec2(point);
                    addLabel(params, Label::Type::point, { p });
                }
            }
        } else {
            addLineTextLabels(_feat, params);
        }
    }

    if (numLabels == m_labels.size()) {
        // Drop quads when no label was added
        m_quads.resize(quadsStart);
    }
    return true;
}

void TextStyleBuilder::addLineTextLabels(const Feature& _feat, const TextStyle::Parameters& _params) {
    float pixelScale = 1.0/m_tileSize;
    float minLength = m_attributes.width * pixelScale;

    float tolerance = pow(pixelScale * 2, 2);

    for (auto& line : _feat.lines) {

        for (size_t i = 0; i < line.size() - 1; i++) {
            glm::vec2 p1 = glm::vec2(line[i]);
            glm::vec2 p2;

            float segmentLength = 0;
            bool merged = false;
            size_t next = i+1;

            for (size_t j = next; j < line.size(); j++) {
                glm::vec2 p = glm::vec2(line[j]);
                segmentLength = glm::length(p1 - p);

                if (j == next) {
                    if (segmentLength > minLength) {
                        addLabel(_params, Label::Type::line, { p1, p });
                    }
                } else {
                    glm::vec2 pp = glm::vec2(line[j-1]);

                    float d = sqPointSegmentDistance(pp, p1, p);
                    if (d > tolerance) { break; }

                    // Skip merged segment
                    merged = true;
                    i += 1;
                }
                p2 = p;
            }

            // place labels at segment-subdivisions
            int run = merged ? 1 : 2;
            segmentLength /= run;

            while (segmentLength > minLength && run <= 4) {
                glm::vec2 a = p1;
                glm::vec2 b = glm::vec2(p2 - p1) / float(run);

                for (int r = 0; r < run; r++) {
                    addLabel(_params, Label::Type::line, { a, a+b });
                    a += b;
                }
                run *= 2;
                segmentLength /= 2.0f;
            }

        }
    }
}

TextStyle::Parameters TextStyleBuilder::applyRule(const DrawRule& _rule,
                                                  const Properties& _props,
                                                  bool _iconText) const {

    const static std::string defaultWeight("400");
    const static std::string defaultStyle("normal");
    const static std::string defaultFamily("default");

    TextStyle::Parameters p;

    _rule.get(StyleParamKey::text_source, p.text);
    if (!_rule.isJSFunction(StyleParamKey::text_source)) {
        if (p.text.empty()) {
            p.text = _props.getString(key_name);
        } else {
            p.text = resolveTextSource(p.text, _props);
        }
    }
    if (p.text.empty()) { return p; }

    auto fontFamily = _rule.get<std::string>(StyleParamKey::text_font_family);
    fontFamily = (!fontFamily) ? &defaultFamily : fontFamily;

    auto fontWeight = _rule.get<std::string>(StyleParamKey::text_font_weight);
    fontWeight = (!fontWeight) ? &defaultWeight : fontWeight;

    auto fontStyle = _rule.get<std::string>(StyleParamKey::text_font_style);
    fontStyle = (!fontStyle) ? &defaultStyle : fontStyle;

    _rule.get(StyleParamKey::text_font_size, p.fontSize);
    p.fontSize *= m_style.pixelScale();

    p.font = m_style.context()->getFont(*fontFamily, *fontStyle, *fontWeight, p.fontSize);

    _rule.get(StyleParamKey::text_font_fill, p.fill);

    _rule.get(StyleParamKey::text_font_stroke_color, p.strokeColor);
    _rule.get(StyleParamKey::text_font_stroke_width, p.strokeWidth);
    p.strokeWidth *= m_style.pixelScale();

    _rule.get(StyleParamKey::transition_hide_time, p.labelOptions.hideTransition.time);
    _rule.get(StyleParamKey::transition_selected_time, p.labelOptions.selectTransition.time);
    _rule.get(StyleParamKey::transition_show_time, p.labelOptions.showTransition.time);

    uint32_t priority;
    if (_iconText) {

        if (_rule.get(StyleParamKey::text_priority, priority)) {
            p.labelOptions.priority = (float)priority;
        }
        _rule.get(StyleParamKey::text_collide, p.labelOptions.collide);
        _rule.get(StyleParamKey::text_interactive, p.interactive);
        _rule.get(StyleParamKey::text_offset, p.labelOptions.offset);
        p.labelOptions.offset *= m_style.pixelScale();

        _rule.get(StyleParamKey::text_anchor, p.labelOptions.anchors);
        if (p.labelOptions.anchors.count == 0) {
            p.labelOptions.anchors.anchor = { LabelProperty::Anchor::bottom, LabelProperty::Anchor::top,
                                              LabelProperty::Anchor::right, LabelProperty::Anchor::left };
            p.labelOptions.anchors.count = 4;
        }

        _rule.get(StyleParamKey::text_transition_hide_time, p.labelOptions.hideTransition.time);
        _rule.get(StyleParamKey::text_transition_selected_time, p.labelOptions.selectTransition.time);
        _rule.get(StyleParamKey::text_transition_show_time, p.labelOptions.showTransition.time);

    } else {
        if (_rule.get(StyleParamKey::priority, priority)) {
            p.labelOptions.priority = (float)priority;
        }
        _rule.get(StyleParamKey::collide, p.labelOptions.collide);
        _rule.get(StyleParamKey::interactive, p.interactive);
        _rule.get(StyleParamKey::offset, p.labelOptions.offset);
        p.labelOptions.offset *= m_style.pixelScale();

        _rule.get(StyleParamKey::anchor, p.labelOptions.anchors);
        if (p.labelOptions.anchors.count == 0) {
            p.labelOptions.anchors.anchor = { LabelProperty::Anchor::center };
            p.labelOptions.anchors.count = 1;
        }
    }

    _rule.get(StyleParamKey::text_wrap, p.maxLineWidth);

    size_t repeatGroupHash = 0;
    std::string repeatGroup;
    if (_rule.get(StyleParamKey::text_repeat_group, repeatGroup)) {
        hash_combine(repeatGroupHash, repeatGroup);
    } else {
        repeatGroupHash = _rule.getParamSetHash();
    }

    StyleParam::Width repeatDistance;
    if (_rule.get(StyleParamKey::text_repeat_distance, repeatDistance)) {
        p.labelOptions.repeatDistance = repeatDistance.value;
    } else {
        p.labelOptions.repeatDistance = View::s_pixelsPerTile;
    }

    hash_combine(repeatGroupHash, p.text);
    p.labelOptions.repeatGroup = repeatGroupHash;
    p.labelOptions.repeatDistance *= m_style.pixelScale();

    if (p.interactive) {
        // TODO optimization: for icon-text use the parent's properties
        p.labelOptions.properties = std::make_shared<Properties>(_props);
    }

    if (auto* transform = _rule.get<std::string>(StyleParamKey::text_transform)) {
        TextLabelProperty::transform(*transform, p.transform);
    }

    if (auto* align = _rule.get<std::string>(StyleParamKey::text_align)) {
        bool res = TextLabelProperty::align(*align, p.align);
        if (!res && p.labelOptions.anchors.count > 0) {
            p.align = TextLabelProperty::alignFromAnchor(p.labelOptions.anchors[0]);
        }
    }

    _rule.get(StyleParamKey::text_required, p.labelOptions.required);

    // TODO style option?
    p.labelOptions.buffer = p.fontSize * 0.25f;

    std::hash<TextStyle::Parameters> hash;
    p.labelOptions.paramHash = hash(p);

    p.lineSpacing = 2 * m_style.pixelScale();

    return p;
}

// TODO use icu transforms
// http://source.icu-project.org/repos/icu/icu/trunk/source/samples/ustring/ustring.cpp

std::string TextStyleBuilder::applyTextTransform(const TextStyle::Parameters& _params,
                                                 const std::string& _string) {
    std::locale loc;
    std::string text = _string;

    switch (_params.transform) {
        case TextLabelProperty::Transform::capitalize:
            text[0] = toupper(text[0], loc);
            if (text.size() > 1) {
                for (size_t i = 1; i < text.length(); ++i) {
                    if (text[i - 1] == ' ') {
                        text[i] = std::toupper(text[i], loc);
                    }
                }
            }
            break;
        case TextLabelProperty::Transform::lowercase:
            for (size_t i = 0; i < text.length(); ++i) {
                text[i] = std::tolower(text[i], loc);
            }
            break;
        case TextLabelProperty::Transform::uppercase:
            // TODO : use to wupper when any wide character is detected
            for (size_t i = 0; i < text.length(); ++i) {
                text[i] = std::toupper(text[i], loc);
            }
            break;
        default:
            break;
    }

    return text;
}

std::string TextStyleBuilder::resolveTextSource(const std::string& textSource,
                                                const Properties& props) const {

    std::string tmp, item;

    // Meaning we have a yaml sequence defining fallbacks
    if (textSource.find(',') != std::string::npos) {
        std::stringstream ss(textSource);

        // Parse fallbacks
        while (std::getline(ss, tmp, ',')) {
            if (props.getAsString(tmp, item)) {
                return item;
            }
        }
    }

    // Fallback to default text source
    if (props.getAsString(textSource, item)) {
        return item;
    }

    // Default to 'name'
    return props.getString(key_name);
}

bool TextStyleBuilder::prepareLabel(TextStyle::Parameters& _params, Label::Type _type) {

    if (_params.text.empty() || _params.fontSize <= 0.f) {
        LOGD("invalid params: '%s' %f", _params.text.c_str(), _params.fontSize);
        return false;
    }

    // Apply text transforms
    const std::string* renderText;
    std::string text;

    if (_params.transform == TextLabelProperty::Transform::none) {
        renderText = &_params.text;
    } else {
        text = applyTextTransform(_params, _params.text);
        renderText = &text;
    }

    // Scale factor by which the texture glyphs are scaled to match fontSize
    _params.fontScale = _params.fontSize / _params.font->size();

    // Stroke width is normalized by the distance of the SDF spread, then
    // scaled to 255 and packed into the "alpha" channel of stroke.
    // Maximal strokeWidth is 3px, attribute is normalized to 0-1 range.

    auto ctx = m_style.context();

    uint32_t strokeAttrib = std::max(_params.strokeWidth / ctx->maxStrokeWidth() * 255.f, 0.f);
    if (strokeAttrib > 255) {
        LOGN("stroke_width too large: %f / %f", _params.strokeWidth, strokeAttrib/255.f);
        strokeAttrib = 255;
    }
    m_attributes.stroke = (_params.strokeColor & 0x00ffffff) + (strokeAttrib << 24);
    m_attributes.fill = _params.fill;
    m_attributes.fontScale = _params.fontScale * 64.f;
    if (m_attributes.fontScale > 255) {
        LOGN("Too large font scale %f, maximal scale is 4", _params.fontScale);
        m_attributes.fontScale = 255;
    }
    m_attributes.quadsStart = m_quads.size();

    m_attributes.textRanges = TextRange{};

    glm::vec2 bbox(0);
    if (ctx->layoutText(_params, *renderText, m_quads, m_atlasRefs, bbox, m_attributes.textRanges)) {
        m_attributes.width = bbox.x;
        m_attributes.height = bbox.y;
        return true;
    }
    return false;
}

void TextStyleBuilder::addLabel(const TextStyle::Parameters& _params, Label::Type _type,
                                Label::Transform _transform) {

    m_labels.emplace_back(new TextLabel(_transform, _type, _params.labelOptions,
                                        {m_attributes.fill, m_attributes.stroke, m_attributes.fontScale},
                                        {m_attributes.width, m_attributes.height},
                                        *m_textLabels, m_attributes.textRanges,
                                        _params.align));
}

}
