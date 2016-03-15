#pragma once

#include "textStyle.h"

#include "labels/textLabel.h"
#include "labels/labelProperty.h"
#include "text/fontContext.h"

#include "alfons/alfons.h"
#include "alfons/textBatch.h"
#include "alfons/textShaper.h"
#include "alfons/lineLayout.h"

#include <memory>
#include <vector>
#include <string>

namespace Tangram {

struct LineWrap {
    alfons::LineMetrics metrics;
    int nbLines;
};

/* Wrap an Alfons line layout, and draw the glyph quads to the TextBatch.
 *
 * This method is not threadsafe!
 *
 * _line the alfons LineLayout containing the glyph shapes and their related codepoints
 * _batch the text batch on which the mesh callback and atlas callback would be triggered
 * _maxChar the maximum line length
 * _minWordLength a parameter to control the minimum word length
 * _alignment align text (center, left, right)
 * _pixelScale the screen pixel density
 */
LineWrap drawWithLineWrapping(const alfons::LineLayout& _line, alfons::TextBatch& _batch,
                              size_t _minLineChars, size_t _maxLineChars,
                              TextLabelProperty::Align _alignment, float _pixelScale);


class TextStyleBuilder : public StyleBuilder,  public alfons::MeshCallback {

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

    // alfons::MeshCallback interface
    void drawGlyph(const alfons::Quad& q, const alfons::AtlasGlyph& altasGlyph) override {}
    void drawGlyph(const alfons::Rect& q, const alfons::AtlasGlyph& atlasGlyph) override;

protected:

    const TextStyle& m_style;

    // Result: TextLabel container
    std::unique_ptr<TextLabels> m_textLabels;

    // Buffers to hold data for TextLabels until build()
    std::vector<GlyphQuad> m_quads;
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

    // TextShaper to create <LineLayout> for a given text
    alfons::TextShaper m_shaper;

    // TextBatch to 'draw' <LineLayout>s, i.e. creating glyph textures and glyph quads.
    // It is intialized with a TextureCallback implemented by FontContext for adding glyph
    // textures and a MeshCallback implemented by TextStyleBuilder for adding glyph quads.
    alfons::TextBatch m_batch;

    float m_tileSize;
    bool m_sdf;
    float m_pixelScale = 1;

};

}
