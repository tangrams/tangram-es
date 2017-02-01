#include "catch.hpp"
#include "tangram.h"
#include "platform_mock.h"
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
    std::shared_ptr<Platform> platform = std::make_shared<MockPlatform>();
    font = fontManager.addFont("default", TEST_FONT_SIZE, alfons::InputSource(_font));

    auto data = platform->bytesFromFile(_font.c_str());
    auto face = fontManager.addFontFace(alfons::InputSource(std::move(data)), TEST_FONT_SIZE);
    font->addFace(face);
}

TEST_CASE("Ensure empty line is given when giving empty shape to alfons", "[Core][Alfons]") {
    initFont();
    auto line = shaper.shape(font, "");

    REQUIRE(line.shapes().size() == 0);
    TextWrapper textWrap;
    alfons::LineMetrics metrics;

    float width = textWrap.getShapeRangeWidth(line, 10, 4);
    int nbLines = textWrap.draw(batch, width, line, TextLabelProperty::Align::center, 1.0, metrics);

    REQUIRE(nbLines == 0);
}

TEST_CASE() {
    initFont();

    auto line = shaper.shape(font, "The quick brown fox");

    REQUIRE(line.shapes().size() == 19);

    TextWrapper textWrap;
    alfons::LineMetrics metrics;
    float width = textWrap.getShapeRangeWidth(line, 4, 10);
    int nbLines = textWrap.draw(batch, width, line, TextLabelProperty::Align::center, 1.0, metrics);
    REQUIRE(nbLines == 2);

    textWrap.clearWraps();
    width = textWrap.getShapeRangeWidth(line, 4, 4);
    nbLines = textWrap.draw(batch, width, line, TextLabelProperty::Align::center, 1.0, metrics);
    REQUIRE(nbLines == 3);

    textWrap.clearWraps();
    width = textWrap.getShapeRangeWidth(line, 0, 1);
    nbLines = textWrap.draw(batch, width, line, TextLabelProperty::Align::center, 1.0, metrics);
    REQUIRE(nbLines == 4);

    textWrap.clearWraps();
    width = textWrap.getShapeRangeWidth(line, 0, 3);
    nbLines = textWrap.draw(batch, width, line, TextLabelProperty::Align::center, 1.0, metrics);
    REQUIRE(nbLines == 4);

    textWrap.clearWraps();
    width = textWrap.getShapeRangeWidth(line, 2, 5);
    nbLines = textWrap.draw(batch, width, line, TextLabelProperty::Align::center, 1.0, metrics);
    REQUIRE(nbLines == 4);
}

TEST_CASE() {
    initFont(TEST_FONT_AR);

    auto line = shaper.shape(font, "لعدم عليها كلّ.");
    REQUIRE(line.shapes().size() == 15);

    TextWrapper textWrap;
    alfons::LineMetrics metrics;

    float width = textWrap.getShapeRangeWidth(line, 0, 1);
    int nbLines = textWrap.draw(batch, width, line, TextLabelProperty::Align::center, 1.0, metrics);
    REQUIRE(nbLines == 3);

    textWrap.clearWraps();
    width = textWrap.getShapeRangeWidth(line, 0, 10);
    nbLines = textWrap.draw(batch, width, line, TextLabelProperty::Align::center, 1.0, metrics);
    REQUIRE(nbLines == 2);
}

TEST_CASE() {
    initFont(TEST_FONT_JP);

    auto line = shaper.shape(font, "日本語のキーボード");
    REQUIRE(line.shapes().size() == 9);

    TextWrapper textWrap;
    alfons::LineMetrics metrics;
    float width = textWrap.getShapeRangeWidth(line, 0, 1);
    int nbLines = textWrap.draw(batch, width, line, TextLabelProperty::Align::center, 1.0, metrics);
    REQUIRE(nbLines == 7);
}

}
