#include "catch.hpp"
#include "tangram.h"
#include "style/textStyle.h"

#include "alfons/textBatch.h"
#include "alfons/textShaper.h"
#include "alfons/atlas.h"
#include "alfons/alfons.h"
#include "alfons/fontManager.h"

#include "text/lineWrapper.h"

#include <memory>

namespace Tangram {

struct ScratchBuffer : public alfons::MeshCallback {
    void drawGlyph(const alfons::Quad& q, const alfons::AtlasGlyph& atlasGlyph) override {}
    void drawGlyph(const alfons::Rect& q, const alfons::AtlasGlyph& atlasGlyph) override {}
};

struct AtlasCallback : public alfons::TextureCallback {
    void addTexture(alfons::AtlasID id, uint16_t textureWidth, uint16_t textureHeight) override {}
    void addGlyph(alfons::AtlasID id, uint16_t gx, uint16_t gy, uint16_t gw, uint16_t gh,
            const unsigned char* src, uint16_t padding) override {}
};

TEST_CASE("Ensure empty line is given when giving empty shape to alfons", "[Core][Alfons]") {
    AtlasCallback atlasCb;
    ScratchBuffer buffer;
    alfons::LineMetrics metrics;
    alfons::TextShaper shaper;
    alfons::GlyphAtlas atlas(atlasCb);
    alfons::TextBatch batch(atlas, buffer);
    alfons::FontManager fontManager;

    auto font = fontManager.addFont("default", alf::InputSource("fonts/NotoSans-Regular.ttf"), 24);

    auto line = shaper.shape(font, "");
    metrics = drawWithLineWrapping(line, batch, 15, TextLabelProperty::Align::center, 1.0);

    REQUIRE(line.shapes().size() == 0);
}

}
