#include "catch.hpp"
#include "tangram.h"
#include "platform.h"
#include "style/textStyleBuilder.h"

#include <memory>

namespace Tangram {

#define TEST_FONT_SIZE  24
#define TEST_FONT       "fonts/NotoSans-Regular.ttf"
#define TEST_FONT_AR 	"fonts/NotoNaskh-Regular.ttf"
#define TEST_FONT_JP    "fonts/DroidSansJapanese.ttf"

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

alfons::TextShaper shaper;
alfons::GlyphAtlas atlas(atlasCb);
alfons::TextBatch batch(atlas, buffer);
alfons::FontManager fontManager;
std::shared_ptr<alfons::Font> font;

void initFont(std::string _font = TEST_FONT) {
    font = fontManager.addFont("default", alfons::InputSource(_font), TEST_FONT_SIZE);

    size_t dataSize = 0;
    unsigned char* data = bytesFromFile(_font.c_str(), dataSize);

    auto face = fontManager.addFontFace(alfons::InputSource(reinterpret_cast<char*>(data), dataSize), TEST_FONT_SIZE);

    font->addFace(face);
}

// TODO: Update

TEST_CASE("Ensure empty line is given when giving empty shape to alfons", "[Core][Alfons]") {
#if 0
    initFont();
    auto line = shaper.shape(font, "");

    REQUIRE(line.shapes().size() == 0);
    TextWrapper textWrap;
    alfons::LineMetrics metrics;
    int nbLines = textWrap.draw(batch, line, 10, 4, TextLabelProperty::Align::center, 1.0, metrics);
    REQUIRE(nbLines == 0);
#endif
}

TEST_CASE() {
#if 0
    initFont();

    auto line = shaper.shape(font, "The quick brown fox");

    REQUIRE(line.shapes().size() == 19);

    TextWrapper textWrap;
    alfons::LineMetrics metrics;
    int nbLines = textWrap.draw(batch, line, 4, 10, TextLabelProperty::Align::center, 1.0, metrics);
    REQUIRE(nbLines == 2);
    nbLines = textWrap.draw(batch, line, 4, 4, TextLabelProperty::Align::center, 1.0, metrics);
    REQUIRE(nbLines == 3);
    nbLines = textWrap.draw(batch, line, 0, 1, TextLabelProperty::Align::center, 1.0, metrics);
    REQUIRE(nbLines == 4);
    nbLines = textWrap.draw(batch, line, 0, 3, TextLabelProperty::Align::center, 1.0, metrics);
    REQUIRE(nbLines == 4);
    nbLines = textWrap.draw(batch, line, 2, 5, TextLabelProperty::Align::center, 1.0, metrics);
    REQUIRE(nbLines == 4);
#endif
}

TEST_CASE() {
#if 0
    initFont(TEST_FONT_AR);

    auto line = shaper.shape(font, "لعدم عليها كلّ.");
    REQUIRE(line.shapes().size() == 15);

    TextWrapper textWrap;
    alfons::LineMetrics metrics;
    int nbLines = textWrap.draw(batch, line, 0, 1, TextLabelProperty::Align::center, 1.0, metrics);
    REQUIRE(nbLines == 3);
    nbLines = textWrap.draw(batch, line, 0, 10, TextLabelProperty::Align::center, 1.0, metrics);
    REQUIRE(nbLines == 2);
#endif
}

TEST_CASE() {
#if 0
    initFont(TEST_FONT_JP);

    auto line = shaper.shape(font, "日本語のキーボード");
    REQUIRE(line.shapes().size() == 9);

    TextWrapper textWrap;
    alfons::LineMetrics metrics;
    int nbLines = textWrap.draw(batch, line, 0, 1, TextLabelProperty::Align::center, 1.0, metrics);
    REQUIRE(nbLines == 7);
#endif
}

}
