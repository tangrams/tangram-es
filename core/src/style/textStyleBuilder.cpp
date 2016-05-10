#include "textStyleBuilder.h"

#include "labels/textLabel.h"
#include "labels/textLabels.h"

#include "data/propertyItem.h" // Include wherever Properties is used!
#include "scene/drawRule.h"
#include "tile/tile.h"
#include "util/geom.h"
#include "util/mapProjection.h"
#include "view/view.h"

#include <cmath>
#include <locale>
#include <mutex>
#include <sstream>

namespace Tangram {

const static std::string key_name("name");


TextStyleBuilder::TextStyleBuilder(const TextStyle& _style)
    : StyleBuilder(_style),
      m_style(_style) {}

void TextStyleBuilder::setup(const Tile& _tile){
    m_tileSize = _tile.getProjection()->TileSize();
    m_quads.clear();
    m_atlasRefs.reset();
    m_labels.clear();

    m_textLabels = std::make_unique<TextLabels>(m_style);
}

std::unique_ptr<StyledMesh> TextStyleBuilder::build() {
    m_textLabels->setLabels(m_labels);
    m_textLabels->setQuads(m_quads, m_atlasRefs);

    return std::move(m_textLabels);
}

void TextStyleBuilder::addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) {
    // TODO: don't reapply applyRule for each subpolygon
    TextStyle::Parameters params = applyRule(_rule, _props);

    // Keep start position of new quads
    size_t quadsStart = m_quads.size();
    size_t numLabels = m_labels.size();

    if (!prepareLabel(params, Label::Type::point)) { return; }

    auto p = centroid(_polygon);
    addLabel(params, Label::Type::point, { p, p });
    
    if (numLabels == m_labels.size()) {
        // Drop quads when no label was added
        m_quads.resize(quadsStart);
    }
}

void TextStyleBuilder::addLine(const Line& _line, const Properties& _props, const DrawRule& _rule) {
    // TODO: don't reapply applyRule for each subpolygon
    TextStyle::Parameters params = applyRule(_rule, _props);

    params.wordWrap = false;
    
    // Keep start position of new quads
    size_t quadsStart = m_quads.size();
    size_t numLabels = m_labels.size();

    if (!prepareLabel(params, Label::Type::line)) { return; }
    
    float pixel = 2.0 / (m_tileSize * m_style.pixelScale());
    float minLength = m_attributes.width * pixel * 0.2;
    
    for (size_t i = 0; i < _line.size() - 1; i++) {
        glm::vec2 p1 = glm::vec2(_line[i]);
        glm::vec2 p2 = glm::vec2(_line[i + 1]);
        if (glm::length(p1-p2) > minLength) {
            addLabel(params, Label::Type::line, { p1, p2 });
        }
    }
    
    if (numLabels == m_labels.size()) {
        // Drop quads when no label was added
        m_quads.resize(quadsStart);
    }
}

void TextStyleBuilder::addPoint(const Point& _point, const Properties& _props, const DrawRule& _rule) {
    // TODO: don't reapply applyRule for each subpolygon
    TextStyle::Parameters params = applyRule(_rule, _props);

    // Keep start position of new quads
    size_t quadsStart = m_quads.size();
    size_t numLabels = m_labels.size();

    if (!prepareLabel(params, Label::Type::point)) { return; }
    
    auto p = glm::vec2(_point);
    addLabel(params, Label::Type::point, { p, p });

    if (numLabels == m_labels.size()) {
        // Drop quads when no label was added
        m_quads.resize(quadsStart);
    }
}

void TextStyleBuilder::addFeature(const Feature& _feat, const DrawRule& _rule) {
    StyleBuilder::addFeature(_feat, _rule);
    return;

#if 0
    TextStyle::Parameters params = applyRule(_rule, _feat.props);

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

    if (!prepareLabel(params, labelType)) { return; }

    if (_feat.geometryType == GeometryType::points) {
        for (auto& point : _feat.points) {
            auto p = glm::vec2(point);
            addLabel(params, Label::Type::point, { p, p });
        }

    } else if (_feat.geometryType == GeometryType::polygons) {
        for (auto& polygon : _feat.polygons) {
            auto p = centroid(polygon);
            addLabel(params, Label::Type::point, { p, p });
        }

    } else if (_feat.geometryType == GeometryType::lines) {

        float pixel = 2.0 / (m_tileSize * m_style.pixelScale());
        float minLength = m_attributes.width * pixel * 0.2;

        for (auto& line : _feat.lines) {
            for (size_t i = 0; i < line.size() - 1; i++) {
                glm::vec2 p1 = glm::vec2(line[i]);
                glm::vec2 p2 = glm::vec2(line[i + 1]);
                if (glm::length(p1-p2) > minLength) {
                    addLabel(params, Label::Type::line, { p1, p2 });
                }
            }
        }
    }

    if (numLabels == m_labels.size()) {
        // Drop quads when no label was added
        m_quads.resize(quadsStart);
    }
#endif

}

TextStyle::Parameters TextStyleBuilder::applyRule(const DrawRule& _rule,
                                                  const Properties& _props) const {

    const static std::string defaultWeight("400");
    const static std::string defaultStyle("normal");
    const static std::string defaultFamily("default");

    TextStyle::Parameters p;
    glm::vec2 offset;

    _rule.get(StyleParamKey::text_source, p.text);
    if (!_rule.isJSFunction(StyleParamKey::text_source)) {
        if (p.text.empty()) {
            p.text = _props.getString(key_name);
        } else {
            p.text = resolveTextSource(p.text, _props);
        }
    }
    if (p.text.empty()) { return p; }

    auto fontFamily = _rule.get<std::string>(StyleParamKey::font_family);
    fontFamily = (!fontFamily) ? &defaultFamily : fontFamily;

    auto fontWeight = _rule.get<std::string>(StyleParamKey::font_weight);
    fontWeight = (!fontWeight) ? &defaultWeight : fontWeight;

    auto fontStyle = _rule.get<std::string>(StyleParamKey::font_style);
    fontStyle = (!fontStyle) ? &defaultStyle : fontStyle;

    _rule.get(StyleParamKey::font_size, p.fontSize);
    p.fontSize *= m_style.pixelScale();

    p.font = m_style.context()->getFont(*fontFamily, *fontStyle, *fontWeight, p.fontSize);

    _rule.get(StyleParamKey::font_fill, p.fill);
    _rule.get(StyleParamKey::offset, p.labelOptions.offset);
    p.labelOptions.offset *= m_style.pixelScale();

    _rule.get(StyleParamKey::font_stroke_color, p.strokeColor);
    _rule.get(StyleParamKey::font_stroke_width, p.strokeWidth);
    p.strokeWidth *= m_style.pixelScale();

    _rule.get(StyleParamKey::priority, p.labelOptions.priority);
    _rule.get(StyleParamKey::collide, p.labelOptions.collide);
    _rule.get(StyleParamKey::transition_hide_time, p.labelOptions.hideTransition.time);
    _rule.get(StyleParamKey::transition_selected_time, p.labelOptions.selectTransition.time);
    _rule.get(StyleParamKey::transition_show_time, p.labelOptions.showTransition.time);
    _rule.get(StyleParamKey::text_wrap, p.maxLineWidth);

    size_t repeatGroupHash = 0;
    std::string repeatGroup;
    if (_rule.get(StyleParamKey::repeat_group, repeatGroup)) {
        hash_combine(repeatGroupHash, repeatGroup);
    } else {
        repeatGroupHash = _rule.getParamSetHash();
    }

    StyleParam::Width repeatDistance;
    if (_rule.get(StyleParamKey::repeat_distance, repeatDistance)) {
        p.labelOptions.repeatDistance = repeatDistance.value;
    } else {
        p.labelOptions.repeatDistance = View::s_pixelsPerTile;
    }

    hash_combine(repeatGroupHash, p.text);
    p.labelOptions.repeatGroup = repeatGroupHash;
    p.labelOptions.repeatDistance *= m_style.pixelScale();

    if (_rule.get(StyleParamKey::interactive, p.interactive) && p.interactive) {
        p.labelOptions.properties = std::make_shared<Properties>(_props);
    }

    if (auto* anchor = _rule.get<std::string>(StyleParamKey::anchor)) {
        LabelProperty::anchor(*anchor, p.anchor);
    }

    if (auto* transform = _rule.get<std::string>(StyleParamKey::transform)) {
        TextLabelProperty::transform(*transform, p.transform);
    }

    if (auto* align = _rule.get<std::string>(StyleParamKey::align)) {
        bool res = TextLabelProperty::align(*align, p.align);
        if (!res) {
            switch(p.anchor) {
            case LabelProperty::Anchor::top_left:
            case LabelProperty::Anchor::left:
            case LabelProperty::Anchor::bottom_left:
                p.align = TextLabelProperty::Align::right;
                break;
            case LabelProperty::Anchor::top_right:
            case LabelProperty::Anchor::right:
            case LabelProperty::Anchor::bottom_right:
                p.align = TextLabelProperty::Align::left;
                break;
            case LabelProperty::Anchor::top:
            case LabelProperty::Anchor::bottom:
            case LabelProperty::Anchor::center:
                break;
            }
        }
    }

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
        // FIXME: This warning should not be logged for every label
        // LOGW("Too large font scale %f, maximal scale is 4", _params.fontScale);
        m_attributes.fontScale = 255;
    }
    m_attributes.quadsStart = m_quads.size();

    glm::vec2 bbox(0);
    if (ctx->layoutText(_params, *renderText, m_quads, m_atlasRefs, bbox)) {
        m_attributes.width = bbox.x;
        m_attributes.height = bbox.y;
        return true;
    }
    return false;
}

void TextStyleBuilder::addLabel(const TextStyle::Parameters& _params, Label::Type _type,
                                Label::Transform _transform) {

    int quadsStart = m_attributes.quadsStart;
    int quadsCount = m_quads.size() - quadsStart;

    m_labels.emplace_back(new TextLabel(_transform, _type, _params.labelOptions, _params.anchor,
                                        {m_attributes.fill, m_attributes.stroke, m_attributes.fontScale},
                                        {m_attributes.width, m_attributes.height},
                                        *m_textLabels, {quadsStart, quadsCount}));
}

}
