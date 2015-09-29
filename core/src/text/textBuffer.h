#pragma once

#include "gl.h"
#include "labels/labelMesh.h"
#include "text/fontContext.h" // for FontID
#include "glm/vec4.hpp"
#include "glm/vec2.hpp"

#include <memory>
#include <locale>
#include <limits>

namespace Tangram {

class FontContext;

enum class TextTransform {
    none,
    capitalize,
    uppercase,
    lowercase,
};

struct Parameters {
    FontID fontId = -1;
    uint32_t fill = 0xff000000;
    uint32_t strokeColor = 0xffffffff;
    float strokeWidth = 0.0f;
    float fontSize = 12.0f;
    float blurSpread = 0.0f;
    TextTransform transform = TextTransform::none;
    bool visible = true;
    uint32_t priority = std::numeric_limits<uint32_t>::max();
    glm::vec2 offset;
    struct {
        bool isFunction = false;
        std::string text;
    } textSource;
};

/*
 * This class holds TextLabels together with their VboMesh
 */
class TextBuffer : public LabelMesh {

public:

    TextBuffer(std::shared_ptr<VertexLayout> _vertexLayout);
    ~TextBuffer();

    /* Create and add TextLabel */
    bool addLabel(const std::string& _text, Label::Transform _transform,
                  Label::Type _type, const Parameters& _params,
                  Label::Options _options, FontContext& _fontContext);

private:


    bool m_dirtyTransform;
};

}
