#pragma once

#include "gl.h"
#include "labels/labelMesh.h"
#include "text/fontContext.h" // for FontID
#include "glm/vec4.hpp"
#include "glm/vec2.hpp"

#include <memory>

namespace Tangram {

class FontContext;

/*
 * This class holds TextLabels together with their VboMesh
 */
class TextBuffer : public LabelMesh {

public:

    TextBuffer(std::shared_ptr<VertexLayout> _vertexLayout);
    ~TextBuffer();

    /* Set <TextBuffer> options for subsequent added labels */
    void init(FontID _fontID, float _size, float _blurSpread);

    /* Create and add TextLabel */
    bool addLabel(const std::string& _text, Label::Transform _transform, Label::Type _type, Label::Options _options);

private:

    FontID m_fontID;
    float m_fontSize;
    float m_fontBlurSpread;

    bool m_dirtyTransform;
};

}
