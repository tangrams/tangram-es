#pragma once

#include "labels/labelSet.h"
#include "style/style.h"
#include "text/fontContext.h"

namespace Tangram {

// Not actually used as VboMesh!
// Just keeps labels and vertices for all Labels of a tile
class LabelContainer : public LabelSet, public StyledMesh {
public:
    LabelContainer(AlfonsContext& _ctx) : context(_ctx) {}

    ~LabelContainer() override {
        context.releaseAtlas(atlasRefs);
    }


    void draw(ShaderProgram& _shader) override {}
    size_t bufferSize() override { return 0; }

    void setLabels(std::vector<std::unique_ptr<Label>>& _labels,
                   std::vector<GlyphQuad>& _quads) {

        typedef std::vector<std::unique_ptr<Label>>::iterator iter_t;

        m_labels.reserve(_labels.size());
        m_labels.insert(m_labels.begin(),
                        std::move_iterator<iter_t>(_labels.begin()),
                        std::move_iterator<iter_t>(_labels.end()));

        quads.reserve(_quads.size());
        quads.insert(quads.begin(),
                     _quads.begin(),
                     _quads.end());

        for (auto& q : quads) {
            atlasRefs.set(q.atlas);
        }

        context.lockAtlas(atlasRefs);
    }

    // TODO: hide within class if needed
    AlfonsContext& context;
    std::vector<GlyphQuad> quads;
    std::bitset<maxTextures> atlasRefs;
};

}
