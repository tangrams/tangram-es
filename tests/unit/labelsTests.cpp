#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"
#include "tangram.h"
#include "platform.h"
#include "debug.h"
#include "scene/scene.h"
#include "style/style.h"
#include "style/textStyle.h"
#include "labels/labels.h"

#include "view/view.h"
#include "tile/tile.h"

namespace Tangram {

glm::vec2 screenSize(256.f, 256.f);
TextBuffer dummy(nullptr);

std::unique_ptr<TextLabel> makeLabel(Label::Transform _transform, Label::Type _type, std::string id) {
    Label::Options options;
    options.color = 0xff;
    options.offset = {0.0f, 0.0f};
    options.id = id;
    return std::unique_ptr<TextLabel>(new TextLabel("label",
                                                    _transform, _type, {0, 0},
                                                    dummy, {0, 0}, options));
}

TEST_CASE("Test getFeaturesAtPoint", "[Labels][FeaturePicking]") {
    std::unique_ptr<Labels> labels(new Labels());

    View view(256, 256);
    view.setPosition(0, 0);
    view.setZoom(0);
    view.update();

    auto labelMesh = std::unique_ptr<LabelMesh>(new LabelMesh(nullptr, 0));
    auto textStyle = std::unique_ptr<TextStyle>(new TextStyle("test"));

    labelMesh->addLabel(makeLabel(glm::vec2{0,0}, Label::Type::point, "0"));
    labelMesh->addLabel(makeLabel(glm::vec2{1,-1}, Label::Type::point, "1"));
    labelMesh->addLabel(makeLabel(glm::vec2{1,1}, Label::Type::point, "2"));

    std::shared_ptr<Tile> tile(new Tile({0,0,0}, view.getMapProjection()));
    tile->getMesh(*textStyle.get()) = std::move(labelMesh);
    tile->update(0, view);

    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(std::move(textStyle));

    std::vector<std::shared_ptr<Tile>> tiles;

    tiles.push_back(tile);
    {
        auto& items = labels->getFeaturesAtPoint(view, 0, styles, tiles, 128, 128);
        REQUIRE(items.size() == 1);
        REQUIRE(items[0].id == "0");
    }
    {
        auto& items = labels->getFeaturesAtPoint(view, 0, styles, tiles, 256, 256);
        REQUIRE(items.size() == 1);
        REQUIRE(items[0].id == "1");
    }

    {
        auto& items = labels->getFeaturesAtPoint(view, 0, styles, tiles, 256, 0);
        REQUIRE(items.size() == 1);
        REQUIRE(items[0].id == "2");
    }
}

}
