#include "debugTextStyle.h"

#include "textStyleBuilder.h"
#include "labels/textLabels.h"

#include "tangram.h"
#include "tile/tile.h"

namespace Tangram {

class DebugTextStyleBuilder : public TextStyleBuilder {

public:

    DebugTextStyleBuilder(const TextStyle& _style)
        : TextStyleBuilder(_style) {}

    void setup(const Tile& _tile) override;

    std::unique_ptr<StyledMesh> build() override;

private:
    std::string m_tileID;

};

void DebugTextStyleBuilder::setup(const Tile& _tile) {
    if (!Tangram::getDebugFlag(Tangram::DebugFlags::tile_infos)) {
        return;
    }

    m_tileID = _tile.getID().toString();

    TextStyleBuilder::setup(_tile);
}

std::unique_ptr<StyledMesh> DebugTextStyleBuilder::build() {
    if (!Tangram::getDebugFlag(Tangram::DebugFlags::tile_infos)) {
        return nullptr;
    }

    TextStyle::Parameters params;

    params.text = m_tileID;
    params.fontSize = 30.f;

    params.font = m_style.context()->getFont("sans-serif", "normal", "400", 32 * m_style.pixelScale());

    if (!prepareLabel(params, Label::Type::debug)) {
        return nullptr;
    }

    addLabel(params, Label::Type::debug, { glm::vec2(.5f) });

    m_textLabels->setLabels(m_labels);

    std::vector<GlyphQuad> quads(m_quads);
    m_textLabels->setQuads(std::move(quads), m_atlasRefs);

    m_quads.clear();
    m_atlasRefs.reset();
    m_labels.clear();

    return std::move(m_textLabels);
}

std::unique_ptr<StyleBuilder> DebugTextStyle::createBuilder() const {
    return std::make_unique<DebugTextStyleBuilder>(*this);
}

}
