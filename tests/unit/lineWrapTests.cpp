#include "catch.hpp"
#include "map.h"
#include "mockPlatform.h"
#include "style/textStyleBuilder.h"

#include "log.h"

#include <memory>

namespace Tangram {

#define TEST_FONT_SIZE  24
#define TEST_FONT       "fonts/NotoSans-Regular.ttf"
#define TEST_FONT_AR    "fonts/NotoNaskh-Regular.ttf"
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
    std::shared_ptr<MockPlatform> platform = std::make_shared<MockPlatform>();
    font = fontManager.addFont("default", TEST_FONT_SIZE, alfons::InputSource(_font));

    auto data = platform->getBytesFromFile(_font.c_str());
    auto face = fontManager.addFontFace(alfons::InputSource(std::move(data)), TEST_FONT_SIZE);
    font->addFace(face);
}

TEST_CASE("Ensure empty line is given when giving empty shape to alfons", "[Core][Alfons]") {
    initFont();
    auto line = shaper.shape(font, "");

    REQUIRE(line.shapes().size() == 0);
    TextWrapper textWrap;
    alfons::LineMetrics metrics;

    float width = textWrap.getShapeRangeWidth(line);
    int nbLines = textWrap.draw(batch, width, line, TextLabelProperty::Align::center, 1.0, metrics);

    REQUIRE(nbLines == 0);
}

TEST_CASE() {
    initFont();

    auto text = UnicodeString::fromUTF8("The quick brown fox");

    {
        auto line = shaper.shapeICU(font, text, 4, 10);
        REQUIRE(line.shapes().size() == 19);

        TextWrapper textWrap;
        alfons::LineMetrics metrics;
        float width = textWrap.getShapeRangeWidth(line);
        int nbLines = textWrap.draw(batch, width, line, TextLabelProperty::Align::center, 1.0, metrics);
        REQUIRE(nbLines == 2);
    }

    {
        auto line = shaper.shapeICU(font, text, 4, 4);
        TextWrapper textWrap;
        alfons::LineMetrics metrics;
        float width = textWrap.getShapeRangeWidth(line);
        int nbLines = textWrap.draw(batch, width, line, TextLabelProperty::Align::center, 1.0, metrics);
        REQUIRE(nbLines == 3);
    }

    {
        auto line = shaper.shapeICU(font, text, 0, 1);

        TextWrapper textWrap;
        alfons::LineMetrics metrics;
        float width = textWrap.getShapeRangeWidth(line);
        int nbLines = textWrap.draw(batch, width, line, TextLabelProperty::Align::center, 1.0, metrics);
        REQUIRE(nbLines == 4);
    }

    {
        auto line = shaper.shapeICU(font, text, 0, 3);
        TextWrapper textWrap;
        alfons::LineMetrics metrics;
        float width = textWrap.getShapeRangeWidth(line);
        int nbLines = textWrap.draw(batch, width, line, TextLabelProperty::Align::center, 1.0, metrics);
        REQUIRE(nbLines == 4);
    }

    {
        auto line = shaper.shapeICU(font, text, 2, 5);
        TextWrapper textWrap;
        alfons::LineMetrics metrics;
        float width = textWrap.getShapeRangeWidth(line);
        int nbLines = textWrap.draw(batch, width, line, TextLabelProperty::Align::center, 1.0, metrics);
        REQUIRE(nbLines == 4);
    }

}

TEST_CASE() {
    initFont(TEST_FONT_AR);

    auto text = UnicodeString::fromUTF8("لعدم عليها كلّ.");

    {
        auto line = shaper.shapeICU(font, text, 0, 1);
        REQUIRE(line.shapes().size() == 15);

        TextWrapper textWrap;
        alfons::LineMetrics metrics;

        float width = textWrap.getShapeRangeWidth(line);
        int nbLines = textWrap.draw(batch, width, line, TextLabelProperty::Align::center, 1.0, metrics);
        REQUIRE(nbLines == 3);
    }

    {
        auto line = shaper.shapeICU(font, text, 0, 10);
        REQUIRE(line.shapes().size() == 15);

        TextWrapper textWrap;
        alfons::LineMetrics metrics;
        float width = textWrap.getShapeRangeWidth(line);
        int nbLines = textWrap.draw(batch, width, line, TextLabelProperty::Align::center, 1.0, metrics);

        REQUIRE(nbLines == 2);
    }
}

TEST_CASE() {
    initFont(TEST_FONT_JP);

    auto text = UnicodeString::fromUTF8("日本語のキーボード");

    auto line = shaper.shapeICU(font, text, 0, 1);
    REQUIRE(line.shapes().size() == 9);

    TextWrapper textWrap;
    alfons::LineMetrics metrics;
    float width = textWrap.getShapeRangeWidth(line);
    int nbLines = textWrap.draw(batch, width, line, TextLabelProperty::Align::center, 1.0, metrics);
    REQUIRE(nbLines == 7);
}


TEST_CASE() {
    initFont(TEST_FONT_AR);

    auto text = UnicodeString::fromUTF8("الضفة الغربية وقطاع غزة");

    {
        auto line = shaper.shapeICU(font, text, 1, 10);
        REQUIRE(line.shapes()[5].mustBreak);
        REQUIRE(line.shapes()[13].mustBreak);
        REQUIRE(line.shapes()[22].mustBreak);
    }

    {
        auto line = shaper.shapeICU(font, text, 1, 15);
        REQUIRE(line.shapes()[13].mustBreak);
        REQUIRE(line.shapes()[22].mustBreak);
    }

}

}
