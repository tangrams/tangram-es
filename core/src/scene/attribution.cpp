#include "data/tileData.h"
#include "labels/labels.h"
#include "marker/markerManager.h"
#include "marker/marker.h"
#include "scene/attribution.h"
#include "scene/scene.h"
#include "style/textStyle.h"

namespace Tangram {

void Attribution::setup(Scene& scene, const std::string& attributionText, const std::string& attributionStyling) {

    m_style = std::make_unique<TextStyle>("attribution-overlay", scene.fontContext(), true);
    m_style->build(scene);
    m_builder = m_style->createBuilder();

    m_marker = std::make_unique<Marker>(0);
    auto feature = std::make_unique<Feature>();
    feature->geometryType = GeometryType::points;
    feature->props.set("name", attributionText);
    auto blah = feature->props.get("name").get<std::string>();
    feature->points.emplace_back();
    m_marker->setFeature(std::move(feature));

    m_marker->setStylingString(attributionStyling);
}

void Attribution::draw(RenderState& rs, const View& view, Scene& scene) {
    m_style->onBeginFrame(rs);
    m_style->onBeginDrawFrame(rs, view, scene);
    m_style->onEndDrawFrame();
}

void Attribution::update(const View& view) {
    if (!m_marker->mesh()) { return; }

    auto* mesh = dynamic_cast<const LabelSet*>(m_marker->mesh());
    m_style->onBeginUpdate();

    for (auto& l : mesh->getLabels()) {
        l->setScreenPosition({10, view.getHeight()});
        l->enterState(Label::State::visible);
        l->evalState(0);
        l->addVerticesToMesh();
    }
}

void Attribution::reset() {
    m_style.reset();
    m_builder.reset();
    m_marker.reset();
}

}
