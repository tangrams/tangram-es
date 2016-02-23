#include "catch.hpp"
#include "tangram.h"
#include "platform.h"
#include "style/textStyle.h"

#include "alfons/textBatch.h"
#include "alfons/textShaper.h"
#include "alfons/atlas.h"
#include "alfons/alfons.h"
#include "alfons/fontManager.h"

#include "text/lineWrapper.h"

#include <memory>

namespace Tangram {

#define TEST_FONT_SIZE  24
#define TEST_FONT       "fonts/NotoSans-Regular.ttf"

struct ScratchBuffer : public alfons::MeshCallback {
    void drawGlyph(const alfons::Quad& q, const alfons::AtlasGlyph& atlasGlyph) override {}
    void drawGlyph(const alfons::Rect& q, const alfons::AtlasGlyph& atlasGlyph) override {}
};

struct AtlasCallback : public alfons::TextureCallback {
    void addTexture(alfons::AtlasID id, uint16_t textureWidth, uint16_t textureHeight) override {}
    void addGlyph(alfons::AtlasID id, uint16_t gx, uint16_t gy, uint16_t gw, uint16_t gh,
            const unsigned char* src, uint16_t padding) override {}
};

AtlasCallback atlasCb;
ScratchBuffer buffer;
LineWrap wrap;
alfons::TextShaper shaper;
alfons::GlyphAtlas atlas(atlasCb);
alfons::TextBatch batch(atlas, buffer);
alfons::FontManager fontManager;
std::shared_ptr<alfons::Font> font;

void initFont() {
    font = fontManager.addFont("default", alf::InputSource(TEST_FONT), TEST_FONT_SIZE);

    unsigned int dataSize = 0;
    unsigned char* data = bytesFromFile(TEST_FONT, PathType::internal, &dataSize);

    auto face = fontManager.addFontFace(alf::InputSource(reinterpret_cast<char*>(data), dataSize), TEST_FONT_SIZE);

    font->addFace(face);
}

TEST_CASE("Ensure empty line is given when giving empty shape to alfons", "[Core][Alfons]") {
    initFont();
    auto line = shaper.shape(font, "");
    REQUIRE(line.shapes().size() == 0);
}

TEST_CASE() {
    initFont();

    auto line = shaper.shape(font, "The quick brown fox");
    wrap = drawWithLineWrapping(line, batch, 10, TextLabelProperty::Align::center, 1.0);

    REQUIRE(line.shapes().size() == 19);
    REQUIRE(wrap.nbLines == 2);
}

TEST_CASE() {
    initFont();

    auto line = shaper.shape(font, "The quick brown fox");
    wrap = drawWithLineWrapping(line, batch, 4, TextLabelProperty::Align::center, 1.0);

    REQUIRE(wrap.nbLines == 4);
}

}
