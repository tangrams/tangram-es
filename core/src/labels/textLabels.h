#pragma once

#include "labels/labelSet.h"
#include "labels/textLabel.h"
#include "text/fontContext.h"

#include <bitset>
#include <vector>

namespace Tangram {

class TextLabels : public LabelSet {

public:

    TextLabels(const TextStyle& _style) : style(_style) {}

    ~TextLabels() override;

    void setQuads(std::vector<GlyphQuad>&& _quads, std::bitset<FontContext::max_textures> _atlasRefs);

    std::vector<GlyphQuad> quads;
    const TextStyle& style;

private:

    std::bitset<FontContext::max_textures> m_atlasRefs;
};

}
