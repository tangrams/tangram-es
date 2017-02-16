#pragma once

#include "labels/textLabel.h"
#include "labels/labelProperty.h"
#include "style/textStyle.h"
#include "text/fontContext.h"

#include <memory>
#include <vector>
#include <string>

namespace Tangram {

class TextStyleBuilder : public StyleBuilder {

public:

    TextStyleBuilder(const TextStyle& _style);

    const Style& style() const override { return m_style; }

    bool addFeature(const Feature& _feature, const DrawRule& _rule) override;

    void setup(const Tile& _tile) override;
    void setup(const Marker& _marker, int zoom) override;

    std::unique_ptr<StyledMesh> build() override;

    TextStyle::Parameters applyRule(const DrawRule& _rule, const Properties& _props, bool _iconText) const;

    bool prepareLabel(TextStyle::Parameters& _params, Label::Type _type);

    // Add label to the mesh using the prepared label data
    void addLabel(const TextStyle::Parameters& _params, Label::Type _type,
                  TextLabel::Coordinates _coordinates, const DrawRule& _rule);

    void addLineTextLabels(const Feature& _feature, const TextStyle::Parameters& _params, const DrawRule& _rule);

    bool addStraightTextLabels(const Line& _feature, const TextStyle::Parameters& _params, const DrawRule& _rule);
    void addCurvedTextLabels(const Line& _feature, const TextStyle::Parameters& _params, const DrawRule& _rule);

    std::string resolveTextSource(const std::string& textSource, const Properties& props) const;

    bool checkRule(const DrawRule& _rule) const override { return true; }

    std::vector<std::unique_ptr<Label>>* labels() { return &m_labels; }

    void addLayoutItems(LabelCollider& _layout) override;

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
        float width = 0;
        float height =0 ;

        // start position in m_quads
        size_t quadsStart = 0;

        uint32_t fill = 0;
        uint32_t stroke = 0;
        uint8_t fontScale = 0;
        TextRange textRanges;
    } m_attributes;

    float m_tileSize = 0;
    float m_tileScale = 0;
};

}
