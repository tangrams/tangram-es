#pragma once

#include "text/fontContext.h"
#include "labels/textLabel.h"
#include "style/style.h"

#include <vector>
#include <bitset>

namespace Tangram {

class TextLabels : public LabelSet, public StyledMesh {

public:

    TextLabels(const TextStyle& _style) : style(_style) {}

    ~TextLabels() override;

    void draw(ShaderProgram& _shader) override {}
    size_t bufferSize() override { return 0; }

    void setQuads(std::vector<GlyphQuad>& _quads);

    std::vector<GlyphQuad> quads;
    const TextStyle& style;

private:

    std::bitset<FontContext::max_textures> m_atlasRefs;
};

}
