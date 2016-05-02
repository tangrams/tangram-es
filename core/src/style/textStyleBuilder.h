#pragma once

#include "textStyle.h"

#include "labels/textLabel.h"
#include "labels/labelProperty.h"
#include "text/fontContext.h"

#include <memory>
#include <vector>
#include <string>

namespace Tangram {

class TextStyleBuilder : public StyleBuilder {

public:

    TextStyleBuilder(const TextStyle& _style);

    const Style& style() const override { return m_style; }

    void addFeature(const Feature& _feature, const DrawRule& _rule) override;

    void setup(const Tile& _tile) override;

    std::unique_ptr<StyledMesh> build() override;

    TextStyle::Parameters applyRule(const DrawRule& _rule, const Properties& _props) const;

    bool prepareLabel(TextStyle::Parameters& _params, Label::Type _type);

    // Add label to the mesh using the prepared label data
    void addLabel(const TextStyle::Parameters& _params, Label::Type _type,
                  Label::Transform _transform);

    std::string applyTextTransform(const TextStyle::Parameters& _params, const std::string& _string);

    std::string resolveTextSource(const std::string& textSource, const Properties& props) const;

    void addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) override;
    void addLine(const Line& _line, const Properties& _props, const DrawRule& _rule) override;
    void addPoint(const Point& _point, const Properties& _props, const DrawRule& _rule) override;

    bool checkRule(const DrawRule& _rule) const override { return true; }

    const std::vector<std::unique_ptr<Label>>& labels() const { return m_labels; }
    const std::vector<GlyphQuad>& quads() const { return m_quads; }

protected:

    const TextStyle& m_style;

    // Result: TextLabel container
    std::unique_ptr<TextLabels> m_textLabels;

    // Buffers to hold data for TextLabels until build()
    std::vector<GlyphQuad> m_quads;
    std::bitset<FontContext::max_textures> m_atlasRefs;
    std::vector<std::unique_ptr<Label>> m_labels;

    // Attributes of the currently prepared Label
    struct {
        float width;
        float height;

        // start position in m_quads
        size_t quadsStart;

        uint32_t fill;
        uint32_t stroke;
        uint8_t fontScale;
    } m_attributes;

    float m_tileSize;
    bool m_sdf;
};

}
