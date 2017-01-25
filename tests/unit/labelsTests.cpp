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

TextStyle dummyStyle("textStyle", nullptr);
TextLabels dummy(dummyStyle);
Label::AABB* bounds = nullptr;

std::unique_ptr<TextLabel> makeLabel(glm::vec2 _transform, Label::Type _type, std::string id) {
    Label::Options options;
    options.offset = {0.0f, 0.0f};
    //options.properties = std::make_shared<Properties>();
    //options.properties->set("id", id);
    options.anchors.anchor[0] = LabelProperty::Anchor::center;
    options.anchors.count = 1;

    return std::unique_ptr<TextLabel>(new TextLabel({{glm::vec3(_transform, 0)}}, _type, options,
                                                    {}, {10, 10}, dummy, {},
                                                    TextLabelProperty::Align::none));
}

TextLabel makeLabelWithAnchorFallbacks(glm::vec2 _transform, glm::vec2 _offset = {0, 0}) {
    Label::Options options;

    // options.anchors.anchor[0] = LabelProperty::Anchor::center;
    options.anchors.anchor[0] = LabelProperty::Anchor::right;
    options.anchors.anchor[1] = LabelProperty::Anchor::bottom;
    options.anchors.anchor[2] = LabelProperty::Anchor::left;
    options.anchors.anchor[3] = LabelProperty::Anchor::top;
    options.anchors.count = 4;

    options.offset = _offset;

    TextRange textRanges;

    return TextLabel({{glm::vec3(_transform, 0)}}, Label::Type::point, options,
            {}, {10, 10}, dummy, textRanges, TextLabelProperty::Align::none);
}

#if 0
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
        auto& items = labels->getFeaturesAtPoint(view.state(), 0, styles, tiles, 128, 128, false);
        REQUIRE(items.size() == 1);
        REQUIRE(items[0].properties->getString("id") == "0");
    }
    {
        auto& items = labels->getFeaturesAtPoint(view.state(), 0, styles, tiles, 256, 256, false);
        REQUIRE(items.size() == 1);
        REQUIRE(items[0].properties->getString("id") == "1");
    }

    {
        auto& items = labels->getFeaturesAtPoint(view.state(), 0, styles, tiles, 256, 0, false);
        REQUIRE(items.size() == 1);
        REQUIRE(items[0].properties->getString("id") == "2");
    }
}
#endif

TEST_CASE( "Test anchor fallback behavior", "[Labels][AnchorFallback]" ) {

    View view(256, 256);
    view.setPosition(0, 0);
    view.setZoom(0);
    view.update(false);

    Tile tile({0,0,0}, view.getMapProjection());
    tile.update(0, view);

    struct TestTransform {
        ScreenTransform transform;
        TestTransform(ScreenTransform::Buffer& _buffer, Range& _range) : transform(_buffer, _range) {}
    };

    class TestLabels : public Labels {
    public:
        TestLabels(View& _v) {
            m_isect2d.resize({1, 1}, {_v.getWidth(), _v.getHeight()});
        }

        ScreenTransform& addLabel(Label* _l, Tile* _t) {
            m_labels.push_back({_l, _t, false, {}});
            tmpTransforms.emplace_back(m_transforms, m_labels.back().transformRange);
            return tmpTransforms.back().transform;
        }
        void run(View& _v) { handleOcclusions(_v.state()); }
        void clear() { m_labels.clear(); }

        std::vector<TestTransform> tmpTransforms;

    };

    {
        TestLabels labels(view);
        TextLabel l1 = makeLabelWithAnchorFallbacks(glm::vec2{0.5,0.5});
        auto& t1 = labels.addLabel(&l1, &tile);
        l1.update(tile.mvp(), view.state(), bounds, t1);

        TextLabel l2 = makeLabelWithAnchorFallbacks(glm::vec2{0.5,0.5});
        auto& t2 = labels.addLabel(&l2, &tile);
        l2.update(tile.mvp(), view.state(), bounds, t2);

        labels.run(view);
        REQUIRE(l1.isOccluded() == false);
        REQUIRE(l2.isOccluded() == true);

        REQUIRE(l1.anchorType() == LabelProperty::Anchor::right);
        REQUIRE(l2.anchorType() == LabelProperty::Anchor::right);
    }

    {
        TestLabels labels(view);
        TextLabel l1 = makeLabelWithAnchorFallbacks(glm::vec2{0.5,0.5});
        auto& t1 = labels.addLabel(&l1, &tile);
        l1.update(tile.mvp(), view.state(), bounds, t1);

        // Second label is one pixel left of L1
        TextLabel l2 = makeLabelWithAnchorFallbacks(glm::vec2{0.5 - 1./256,0.5});
        auto& t2 = labels.addLabel(&l2, &tile);
        l2.update(tile.mvp(), view.state(), bounds, t2);

        labels.run(view);
        // l1.print();
        // l2.print();
        REQUIRE(l1.isOccluded() == false);
        REQUIRE(l2.isOccluded() == false);

        REQUIRE(l1.anchorType() == LabelProperty::Anchor::right);
        // Check that left-of anchor fallback is used
        REQUIRE(l2.anchorType() == LabelProperty::Anchor::left);
    }

    {
        TestLabels labels(view);
        TextLabel l1 = makeLabelWithAnchorFallbacks(glm::vec2{0.5,0.5});
        auto& t1 = labels.addLabel(&l1, &tile);
        l1.update(tile.mvp(), view.state(), bounds, t1);

        // Second label is 10 pixel top of L1
        TextLabel l2 = makeLabelWithAnchorFallbacks(glm::vec2{0.5,0.5 + 10./256});
        auto& t2 = labels.addLabel(&l2, &tile);
        l2.update(tile.mvp(), view.state(), bounds, t2);

        labels.run(view);
        REQUIRE(l1.isOccluded() == false);
        REQUIRE(l2.isOccluded() == false);

        REQUIRE(l1.anchorType() == LabelProperty::Anchor::right);
        // Check that anchor fallback is used
        REQUIRE(l2.anchorType() == LabelProperty::Anchor::top);
    }

    {
        TestLabels labels(view);
        TextLabel l1 = makeLabelWithAnchorFallbacks(glm::vec2{0.5,0.5});
        auto& t1 = labels.addLabel(&l1, &tile);
        l1.update(tile.mvp(), view.state(), bounds, t1);

        // Second label is 10 pixel below of L1
        TextLabel l2 = makeLabelWithAnchorFallbacks(glm::vec2{0.5,0.5 - 10./256});
        auto& t2 = labels.addLabel(&l2, &tile);
        l2.update(tile.mvp(), view.state(), bounds, t2);

        labels.run(view);
        REQUIRE(l1.isOccluded() == false);
        REQUIRE(l2.isOccluded() == false);

        REQUIRE(l1.anchorType() == LabelProperty::Anchor::right);
        // Check that anchor fallback is used
        REQUIRE(l2.anchorType() == LabelProperty::Anchor::bottom);
    }

}
}
