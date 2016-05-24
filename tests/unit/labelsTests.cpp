#include "catch.hpp"
#include "tangram.h"
#include "platform.h"
#include "scene/scene.h"
#include "style/style.h"
#include "style/textStyle.h"
#include "labels/labels.h"
#include "labels/textLabel.h"
#include "labels/textLabels.h"
#include "gl/dynamicQuadMesh.h"

#include "view/view.h"
#include "tile/tile.h"

#include <memory>

namespace Tangram {

glm::vec2 screenSize(256.f, 256.f);
TextStyle dummyStyle("textStyle", nullptr);
TextLabels dummy(dummyStyle);

std::unique_ptr<TextLabel> makeLabel(Label::Transform _transform, Label::Type _type, std::string id) {
    Label::Options options;
    options.offset = {0.0f, 0.0f};
    options.properties = std::make_shared<Properties>();
    options.properties->set("id", id);
    options.interactive = true;

    return std::unique_ptr<TextLabel>(new TextLabel(_transform, _type, options,
            LabelProperty::Anchor::center,
            {}, {10, 10}, dummy, {}));
}

TEST_CASE("Test getFeaturesAtPoint", "[Labels][FeaturePicking]") {
    std::unique_ptr<Labels> labels(new Labels());

    View view(256, 256);
    view.setPosition(0, 0);
    view.setZoom(0);
    view.update(false);

    struct TestLabelMesh : public LabelSet {
        void addLabel(std::unique_ptr<Label> _label) { m_labels.push_back(std::move(_label)); }
    };

    auto labelMesh = std::unique_ptr<TestLabelMesh>(new TestLabelMesh());
    auto textStyle = std::unique_ptr<TextStyle>(new TextStyle("test", nullptr, false));
    textStyle->setID(0);

    labelMesh->addLabel(makeLabel(glm::vec2{.5f,.5f}, Label::Type::point, "0"));
    labelMesh->addLabel(makeLabel(glm::vec2{1,0}, Label::Type::point, "1"));
    labelMesh->addLabel(makeLabel(glm::vec2{1,1}, Label::Type::point, "2"));

    std::shared_ptr<Tile> tile(new Tile({0,0,0}, view.getMapProjection()));
    tile->initGeometry(1);
    tile->setMesh(*textStyle.get(), std::move(labelMesh));
    tile->update(0, view);

    std::vector<std::unique_ptr<Style>> styles;
    styles.push_back(std::move(textStyle));

    std::vector<std::shared_ptr<Tile>> tiles;

    tiles.push_back(tile);
    {
        auto& items = labels->getFeaturesAtPoint(view, 0, styles, tiles, 128, 128, false);
        REQUIRE(items.size() == 1);
        REQUIRE(items[0].properties->getString("id") == "0");
    }
    {
        auto& items = labels->getFeaturesAtPoint(view, 0, styles, tiles, 256, 256, false);
        REQUIRE(items.size() == 1);
        REQUIRE(items[0].properties->getString("id") == "1");
    }

    {
        auto& items = labels->getFeaturesAtPoint(view, 0, styles, tiles, 256, 0, false);
        REQUIRE(items.size() == 1);
        REQUIRE(items[0].properties->getString("id") == "2");
    }
}

}
