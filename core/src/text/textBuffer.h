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
struct Properties;

enum class TextTransform {
    none,
    capitalize,
    uppercase,
    lowercase,
};

struct Parameters {
    FontID fontId = -1;
    std::string text = "";
    bool interactive = false;
    std::shared_ptr<Properties> properties;
    uint32_t fill = 0xff000000;
    uint32_t strokeColor = 0xffffffff;
    float strokeWidth = 0.0f;
    float fontSize = 12.0f;
    float blurSpread = 0.0f;
    bool visible = true;
    bool collide = true;
    TextTransform transform = TextTransform::none;
    uint32_t priority = std::numeric_limits<uint32_t>::max();
    glm::vec2 offset;

    bool isValid() {
        return fontSize > 0.f && !text.empty();
    }
};

/*
 * This class holds TextLabels together with their VboMesh
 */
class TextBuffer : public LabelMesh {

public:

    TextBuffer(std::shared_ptr<VertexLayout> _vertexLayout);
    ~TextBuffer();

    /* Create and add TextLabel */
    bool addLabel(const Parameters& _params, Label::Transform _transform,
                  Label::Type _type, FontContext& _fontContext);

private:


    bool m_dirtyTransform;
};

}
