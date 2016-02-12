#pragma once

#include "labels/label.h"
#include "style/labelProperty.h"

#include "labels/labelSet.h"
#include "labels/labelMesh.h"
#include "style/textStyle.h"
#include "text/fontContext.h"

#include <bitset>
#include <vector>

namespace Tangram {

class TextLabels;

struct FontMetrics {
    float ascender, descender, lineHeight;
};

class TextLabel : public Label {
public:
    TextLabel(Label::Transform _transform, Type _type,
              glm::vec2 _dim, TextLabels& _labels,
              Range _vertexRange,
              Label::Options _options,
              FontMetrics _metrics,
              int _nLines, LabelProperty::Anchor _anchor,
              glm::vec2 _quadsLocalOrigin);

    void updateBBoxes(float _zoomFract) override;

protected:

    void align(glm::vec2& _screenPosition, const glm::vec2& _ap1, const glm::vec2& _ap2) override;
    FontMetrics m_metrics;
    int m_nLines;

    void pushTransform() override;

private:
    // Back-pointer to owning container
    TextLabels& m_textLabels;

    glm::vec2 m_anchor;
    glm::vec2 m_quadLocalOrigin;
};


class TextLabels : public LabelSet, public StyledMesh {
public:
    TextLabels(const TextStyle& _style) : m_style(_style) {}

    ~TextLabels() override {
        m_style.context()->releaseAtlas(atlasRefs);
    }

    void draw(ShaderProgram& _shader, bool _clear) override {}
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
