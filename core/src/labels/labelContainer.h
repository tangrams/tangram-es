#pragma once

#include "labels/labelSet.h"
#include "style/textStyle.h"
#include "text/fontContext.h"
#include "text/textMesh.h"

namespace Tangram {

// Not actually used as VboMesh!
// Just keeps labels and vertices for all Labels of a tile
class LabelContainer : public LabelSet, public StyledMesh {
public:
    LabelContainer(const TextStyle& _style) : m_style(_style) {}

    ~LabelContainer() override {
        m_style.context()->releaseAtlas(atlasRefs);
    }

    void draw(ShaderProgram& _shader) override {}
    size_t bufferSize() override { return 0; }

    void setQuads(std::vector<GlyphQuad>& _quads) {

        quads.insert(quads.end(),
                     _quads.begin(),
                     _quads.end());

        for (auto& q : quads) { atlasRefs.set(q.atlas); }
        m_style.context()->lockAtlas(atlasRefs);
    }

    // TODO: hide within class if needed
    const TextStyle& m_style;
    std::vector<GlyphQuad> quads;
    std::bitset<maxTextures> atlasRefs;
};

}
