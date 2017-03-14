#include "style/textStyleBuilder.h"

#include "labels/curvedLabel.h"
#include "labels/labelCollider.h"
#include "labels/labelSet.h"
#include "labels/textLabel.h"
#include "labels/textLabels.h"
#include "log.h"
#include "marker/marker.h"
#include "selection/featureSelection.h"
#include "scene/drawRule.h"
#include "tangram.h"
#include "tile/tile.h"
#include "util/geom.h"
#include "util/mapProjection.h"
#include "util/lineSampler.h"
#include "view/view.h"

#include "unicode/unistr.h"
#include "unicode/schriter.h"
#include "unicode/brkiter.h"
#include "unicode/locid.h"

#include <glm/gtx/norm.hpp>
#include <algorithm>
#include <cmath>
#include <locale>
#include <mutex>

namespace Tangram {

const static std::string key_name("name");

TextStyleBuilder::TextStyleBuilder(const TextStyle& _style) : m_style(_style) {}

void TextStyleBuilder::setup(const Tile& _tile){
    m_tileSize = _tile.getProjection()->TileSize();
    m_tileSize *= m_style.pixelScale();

    // < 1.0 when overzooming a tile
    m_tileScale = pow(2, _tile.getID().s - _tile.getID().z);
    m_tileSize *= m_tileScale;

    m_atlasRefs.reset();

    m_textLabels = std::make_unique<TextLabels>(m_style);
}

void TextStyleBuilder::setup(const Marker& marker, int zoom) {
    float metersPerTile = 2.f * MapProjection::HALF_CIRCUMFERENCE * exp2(-zoom);

    // In general, a Marker won't cover the same area as a tile, so the effective
    // "tile size" for building a Marker is the size of a tile in pixels multiplied
    // by the ratio of the Marker's extent to the length of a tile side at this zoom.
    m_tileSize = 256 * (marker.extent() / metersPerTile);

    // (Copied from Tile setup function above, purpose unclear)
    m_tileSize *= m_style.pixelScale();

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

            const auto& ranges = textLabel->textRanges();

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

bool TextStyleBuilder::addFeature(const Feature& _feat, const DrawRule& _rule) {
    TextStyle::Parameters params = applyRule(_rule, _feat.props, false);

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
            addLabel(params, Label::Type::point, {{ p, p }}, _rule);
        }

    } else if (_feat.geometryType == GeometryType::polygons) {

        const auto& polygons = _feat.polygons;
        for (const auto& polygon : polygons) {
            if (!polygon.empty()) {
                glm::vec3 c;
                c = centroid(polygon.front().begin(), polygon.front().end());
                addLabel(params, Label::Type::point, {{ c }}, _rule);
            }
        }
    } else if (_feat.geometryType == GeometryType::lines) {
        addLineTextLabels(_feat, params, _rule);
    }

    if (numLabels == m_labels.size()) {
        // Drop quads when no label was added
        m_quads.resize(quadsStart);
    }
    return true;
}

bool TextStyleBuilder::addStraightTextLabels(const Line& _line, const TextStyle::Parameters& _params,
                                             const DrawRule& _rule) {

    // Size of pixel in tile coordinates
    float pixelSize = 1.0/m_tileSize;

    // Minimal length of line needed for the label
    float minLength = m_attributes.width * pixelSize;

    // Allow labels to appear later than tile's min-zoom
    minLength *= 0.6;

    //float tolerance = pow(pixelScale * 2, 2);
    float tolerance = pow(pixelSize * 1.5, 2);
    float sqDirLimit = powf(1.99f, 2);

    for (size_t i = 0; i < _line.size() - 1; i++) {
        glm::vec2 p0 = glm::vec2(_line[i]);
        glm::vec2 p1 = glm::vec2(_line[i+1]);

        float segmentLength = glm::length(p0 - p1);

        glm::vec2 dir0 = (p0 - p1) / segmentLength;
        glm::vec2 dir1 = dir0;
        glm::vec2 dir2;

        int merged = 0;

        size_t j = i + 2;
        for (; j < _line.size(); j++) {
            glm::vec2 p2 = glm::vec2(_line[j]);

            segmentLength = glm::length(p1 - p2);
            dir2 = (p1 - p2) / segmentLength;

            glm::vec2 pp = glm::vec2(_line[j-1]);
            float d = sqPointSegmentDistance(pp, p0, p2);
            if (d > tolerance) { break; }

            if ((glm::length2(dir1 + dir2) < sqDirLimit) ||
                (glm::length2(dir0 + dir2) < sqDirLimit)) {
                break;
            }

            merged++;

            p1 = p2;
            dir1 = dir2;
        }

        // place labels at segment-subdivisions
        int run = 1;
        if (merged) { segmentLength = glm::length(p0 - p1); }

        while (segmentLength > minLength && run <= 4) {
            glm::vec2 a = p0;
            glm::vec2 b = glm::vec2(p1 - p0) / float(run);

            for (int r = 0; r < run; r++) {
                addLabel(_params, Label::Type::line, {{ a, a+b }}, _rule);
                a += b;
            }
            run *= 2;
            segmentLength /= 2.0f;
        }

        if (i == 0 && j == _line.size()) {
            // Simple straight line
            return true;
        }

        // Skip merged segments in outer loop
        i += merged;

    }
    return false;
}

void TextStyleBuilder::addCurvedTextLabels(const Line& _line, const TextStyle::Parameters& _params,
                                           const DrawRule& _rule) {

    // Size of pixel in tile coordinates
    const float pixelSize = 1.0/m_tileSize;
    // length of line needed for the label
    const float labelLength = m_attributes.width * pixelSize;
    // Allow labels to appear later than tile's min-zoom
    const float minLength = labelLength * 0.6;

    // Chord length for minimal ~120 degree inner angles (squared)
    // sin(60)*2
    const float sqDirLimit = powf(1.7f, 2);
    // Range to check for angle changes
    const float sampleWindow = pixelSize * 50;

    // Minimal ~10 degree counts as change of direction
    // cross(dir1,dir2) < sin(10)
    const float flipTolerance = 0.17;

    LineSampler<std::vector<glm::vec3>> sampler;

    sampler.set(_line);

    if (sampler.sumLength() < minLength) { return; }

    struct LineRange {
        size_t start, end;
        int flips;
        float sumAngle;
    };

    std::vector<LineRange> ranges;

    for (size_t i = 0; i < _line.size()-1; i++) {

        int flips = 0;
        float lastAngle = 0;
        float sumAngle = 0;
        size_t lastBreak = 0;

        glm::vec2 dir1 = sampler.segmentDirection(i);

        for (size_t j = i + 1; j < _line.size()-1; j++) {
            glm::vec2 dir2 = sampler.segmentDirection(j);
            bool splitLine = false;

            if (glm::length2(dir1 + dir2) < sqDirLimit) {
                // Split if the angle between current and next segment is
                // not whithin 120 < a < 240 degree
                splitLine = true;
            } else {
                // Take cross product of direction (unit-) vectors of the current
                // and next segment. The magnitude of the cross product is the sine
                // angle between dir1 and dir2.
                float angle = crossProduct(dir1, dir2);

                if (std::abs(angle) > flipTolerance) {
                    if (lastAngle > 0) {
                        if (angle < 0) { flips++; }
                    } else if (lastAngle < 0) {
                        if (angle > 0) { flips++; }
                    }
                    lastAngle = angle;
                }

                // Limit number of direction changes (Avoid squiggly labels)
                if (flips > 2) {
                    splitLine = true;
                } else {
                    sumAngle += std::abs(angle);
                }
            }

            if (!splitLine) {
                // Go back within window to check for hard direction changes
                for (int k = j - 1; k >= int(i); k--){
                    if (glm::length2(sampler.segmentDirection(k) + dir2) < sqDirLimit) {
                        splitLine = true;
                    }
                    if (sampler.point(k).z < sampler.point(j).z - sampleWindow) {
                        break;
                    }
                }
            }

            if (splitLine) {
                float length = sampler.point(j).z - sampler.point(i).z;
                if (length > minLength) {
                    ranges.push_back(LineRange{i, j+1, flips, sumAngle});
                }

                lastBreak = j;
                break;

            } else {
                dir1 = dir2;
            }
        }

        // Add segment from 'i' unless line got split.
        if (lastBreak == 0) {
            float length = sampler.sumLength() - sampler.point(i).z;
            if (length > minLength) {
                ranges.push_back(LineRange{i, _line.size(), flips, sumAngle});
            }
        }
    }

    for (auto& range : ranges) {
        glm::vec2 center;
        glm::vec2 rotation;
        float startLen = sampler.point(range.start).z;
        float length = (sampler.point(range.end-1).z - startLen);
        float mid = startLen + length * 0.5;

        sampler.sample(mid, center, rotation);
        size_t offset = sampler.curSegment();

        std::vector<glm::vec2> l;
        l.reserve(range.end - range.start + 1);

        for (size_t j = range.start; j < range.end; j++) {
            auto& p = _line[j];
            l.emplace_back(p.x, p.y);

            if (j == offset) {
                l.push_back(center);
            }
        }
        size_t anchor = offset - range.start + 1;

        // NB: Just some heuristic to prefer longer and less curvy parts..
        float prio = (1.f + range.sumAngle) / length;

        uint32_t selectionColor = 0;
        if (_params.interactive) {
            selectionColor = _rule.featureSelection->nextColorIdentifier();
        }

        m_labels.emplace_back(new CurvedLabel(l, _params.labelOptions, prio,
                                               {m_attributes.fill,
                                                       m_attributes.stroke,
                                                       m_attributes.fontScale,
                                                       selectionColor},
                                               {m_attributes.width, m_attributes.height},
                                               *m_textLabels, m_attributes.textRanges,
                                               TextLabelProperty::Align::center,
                                               anchor));

    }
}

void TextStyleBuilder::addLineTextLabels(const Feature& _feat, const TextStyle::Parameters& _params,
                                         const DrawRule& _rule) {

    for (auto& line : _feat.lines) {

        if (!addStraightTextLabels(line, _params, _rule) &&
            !_params.hasComplexShaping && line.size() > 2) {
            addCurvedTextLabels(line, _params, _rule);
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
    size_t repeatGroupHash = 0;
    std::string repeatGroup;
    StyleParam::Width repeatDistance;
    glm::vec2 defaultBuffer = glm::vec2(p.fontSize * 0.25f);

    if (_iconText) {
        if (_rule.get(StyleParamKey::text_priority, priority)) {
            p.labelOptions.priority = (float)priority;
        }
        _rule.get(StyleParamKey::text_collide, p.labelOptions.collide);
        if (!_rule.get(StyleParamKey::text_interactive, p.interactive)) {
            _rule.get(StyleParamKey::interactive, p.interactive);
        }
        _rule.get(StyleParamKey::text_offset, p.labelOptions.offset);
        p.labelOptions.offset *= m_style.pixelScale();

        _rule.get(StyleParamKey::text_anchor, p.labelOptions.anchors);
        if (p.labelOptions.anchors.count == 0) {
            p.labelOptions.anchors.anchor = { {LabelProperty::Anchor::bottom, LabelProperty::Anchor::top,
                                               LabelProperty::Anchor::right, LabelProperty::Anchor::left} };
            p.labelOptions.anchors.count = 4;
        }

        // child text's repeat group params
        if (_rule.get(StyleParamKey::text_repeat_distance, repeatDistance)) {
            p.labelOptions.repeatDistance = repeatDistance.value;
        } else {
            p.labelOptions.repeatDistance = View::s_pixelsPerTile;
        }

        if (p.labelOptions.repeatDistance > 0.f) {
            if (_rule.get(StyleParamKey::text_repeat_group, repeatGroup)) {
                hash_combine(repeatGroupHash, repeatGroup);
            } else if (_rule.get(StyleParamKey::repeat_group, repeatGroup)) { //inherit from parent point
                hash_combine(repeatGroupHash, repeatGroup);
            } else {
                repeatGroupHash = _rule.getParamSetHash();
            }
        }

        _rule.get(StyleParamKey::text_transition_hide_time, p.labelOptions.hideTransition.time);
        _rule.get(StyleParamKey::text_transition_selected_time, p.labelOptions.selectTransition.time);
        _rule.get(StyleParamKey::text_transition_show_time, p.labelOptions.showTransition.time);

        if (!_rule.get(StyleParamKey::text_buffer, p.labelOptions.buffer)) {
            p.labelOptions.buffer = defaultBuffer;
        }
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
            p.labelOptions.anchors.anchor = { {LabelProperty::Anchor::center} };
            p.labelOptions.anchors.count = 1;
        }

        if (_rule.get(StyleParamKey::repeat_distance, repeatDistance)) {
            p.labelOptions.repeatDistance = repeatDistance.value;
        } else {
            p.labelOptions.repeatDistance = View::s_pixelsPerTile;
        }

        if (p.labelOptions.repeatDistance > 0.f) {
            if (_rule.get(StyleParamKey::repeat_group, repeatGroup)) {
                hash_combine(repeatGroupHash, repeatGroup);
            } else {
                repeatGroupHash = _rule.getParamSetHash();
            }
        }

        if (!_rule.get(StyleParamKey::buffer, p.labelOptions.buffer)) {
            p.labelOptions.buffer = defaultBuffer;
        }
    }

    if (p.labelOptions.repeatDistance > 0.f) {
        hash_combine(repeatGroupHash, p.text);
        p.labelOptions.repeatGroup = repeatGroupHash;
        p.labelOptions.repeatDistance *= m_style.pixelScale();
    }

    _rule.get(StyleParamKey::text_wrap, p.maxLineWidth);

    if (auto* transform = _rule.get<std::string>(StyleParamKey::text_transform)) {
        TextLabelProperty::transform(*transform, p.transform);
    }

    if (auto* align = _rule.get<std::string>(StyleParamKey::text_align)) {
        bool res = TextLabelProperty::align(*align, p.align);
        if (!res && p.labelOptions.anchors.count > 0) {
            p.align = TextLabelProperty::alignFromAnchor(p.labelOptions.anchors[0]);
        }
    }

    _rule.get(StyleParamKey::text_optional, p.labelOptions.optional);

    std::hash<TextStyle::Parameters> hash;
    p.labelOptions.paramHash = hash(p);

    p.lineSpacing = 2 * m_style.pixelScale();

    if (p.interactive) {
        p.labelOptions.featureId = _rule.selectionColor;
    }

    return p;
}

void applyTextTransform(const TextStyle::Parameters& _params,
                        icu::UnicodeString& _string) {

    icu::Locale loc("en");

    switch (_params.transform) {
    case TextLabelProperty::Transform::capitalize: {
        UErrorCode status{U_ZERO_ERROR};
        auto *wordIterator = BreakIterator::createWordInstance(loc, status);

        if (U_SUCCESS(status)) { _string.toTitle(wordIterator); }

        delete wordIterator;
        break;
    }
    case TextLabelProperty::Transform::lowercase:
        _string.toLower(loc);
        break;
    case TextLabelProperty::Transform::uppercase:
        _string.toUpper(loc);
        break;
    default:
        break;
    }
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

bool isComplexShapingScript(const icu::UnicodeString& _text) {

    // Taken from:
    // https://github.com/tangrams/tangram/blob/labels-rebase/src/styles/text/canvas_text.js#L538-L553
    // See also http://r12a.github.io/scripts/featurelist/

    icu::StringCharacterIterator iterator(_text);
    for (UChar c = iterator.first(); c != CharacterIterator::DONE; c = iterator.next()) {
        if (c >= u'\u0600' && c <= u'\u18AF') {
            if ((c <= u'\u06FF') ||                   // Arabic:     "\u0600-\u06FF"
                (c >= u'\u0900' && c <= u'\u097F') || // Devanagari: "\u0900-\u097F"
                (c >= u'\u0980' && c <= u'\u09FF') || // Bengali:    "\u0980-\u09FF"
                (c >= u'\u0A00' && c <= u'\u0A7F') || // Gurmukhi:   "\u0A00-\u0A7F"
                (c >= u'\u0A80' && c <= u'\u0AFF') || // Gujarati:   "\u0A80-\u0AFF"
                (c >= u'\u0B00' && c <= u'\u0B7f') || // Oriya:      "\u0B00-\u0B7F"
                (c >= u'\u0B80' && c <= u'\u0BFF') || // Tamil:      "\u0B80-\u0BFF"
                (c >= u'\u0C00' && c <= u'\u0C7F') || // Telugu:     "\u0C00-\u0C7F"
                (c >= u'\u0E80' && c <= u'\u0EFF') || // Lao:        "\u0E80-\u0EFF"
                (c >= u'\u0F00' && c <= u'\u0FFF') || // Tibetan:    "\u0F00-\u0FFF"
                (c >= u'\u1000' && c <= u'\u109F') || // Burmese:    "\u1000-\u109F"
                (c >= u'\u1780' && c <= u'\u17FF') || // Khmer:      "\u1780-\u17FF"
                (c >= u'\u1800' && c <= u'\u18AF')) { // Mongolian:  "\u1800-\u18AF"
                return true;
            }
        }
    }
    return false;
}

bool TextStyleBuilder::prepareLabel(TextStyle::Parameters& _params, Label::Type _type) {

    if (_params.text.empty() || _params.fontSize <= 0.f) {
        // Nothing to render!
        return false;
    }

    auto text = icu::UnicodeString::fromUTF8(_params.text);

    applyTextTransform(_params, text);

    if (_type == Label::Type::line) {
        _params.hasComplexShaping = isComplexShapingScript(text);
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
    m_attributes.fontScale = std::min(int(_params.fontScale * 64.f), 255);
    m_attributes.quadsStart = m_quads.size();
    m_attributes.textRanges = TextRange{};

    glm::vec2 bbox(0);
    if (ctx->layoutText(_params, text, m_quads, m_atlasRefs, bbox, m_attributes.textRanges)) {

        int start = m_attributes.quadsStart;
        for (auto& range : m_attributes.textRanges) {
            assert(range.start == start);
            assert(range.length >= 0);
            start += range.length;
        }
        m_attributes.width = bbox.x;
        m_attributes.height = bbox.y;
        return true;
    }

    return false;
}

void TextStyleBuilder::addLabel(const TextStyle::Parameters& _params, Label::Type _type,
                                TextLabel::Coordinates _coordinates, const DrawRule& _rule) {

    uint32_t selectionColor = 0;

    if (_params.interactive) {
        selectionColor = _rule.featureSelection->nextColorIdentifier();
    }

    m_labels.emplace_back(new TextLabel(_coordinates, _type, _params.labelOptions,
                                        {m_attributes.fill,
                                         m_attributes.stroke,
                                         m_attributes.fontScale,
                                         selectionColor},
                                        {m_attributes.width, m_attributes.height},
                                        *m_textLabels, m_attributes.textRanges,
                                        _params.align));
}

}
