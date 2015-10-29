#pragma once

#include "gl.h"
#include "labels/labelMesh.h"
#include "text/fontContext.h" // for FontID
#include "glm/vec4.hpp"
#include "glm/vec2.hpp"
#include "style/textStyle.h"

#include <memory>
#include <locale>
#include <limits>

namespace Tangram {

class FontContext;

/*
 * This class holds TextLabels together with their VboMesh
 */
class TextBuffer : public LabelMesh {

public:

    TextBuffer(std::shared_ptr<VertexLayout> _vertexLayout);
    ~TextBuffer();

    /* Create and add TextLabel */
    bool addLabel(const TextStyle::Parameters& _params, Label::Transform _transform,
                  Label::Type _type, FontContext& _fontContext);

    glm::vec2 getAtlasResolution() { return m_atlasRes; }

private:

    glm::vec2 m_atlasRes;
    bool m_dirtyTransform;
};

}
