#pragma once

#include "gl.h"
#include "labels/labelMesh.h"
#include "text/fontContext.h" // for FontID
#include "glm/vec4.hpp"
#include "glm/vec2.hpp"

#include <memory>
#include <locale>

namespace Tangram {

class FontContext;

struct Parameters {
    std::string fontKey = "";
    uint32_t fill = 0xff000000;
    uint32_t strokeColor = 0xffffffff;
    float strokeWidth = 0.0f;
    float fontSize = 12.0f;
    float blurSpread = 0.0f;
    bool uppercase = false;
    bool visible = true;
    uint32_t priority = 1 << 31;
    glm::vec2 offset;
};

/*
 * This class holds TextLabels together with their VboMesh
 */
class TextBuffer : public LabelMesh {

public:

    TextBuffer(std::shared_ptr<VertexLayout> _vertexLayout);
    ~TextBuffer();

    /* Create and add TextLabel */
    bool addLabel(TileID _tileID, const std::string& _text, Label::Transform _transform, Label::Type _type, const Parameters& _params, Label::Options _options);

private:


    bool m_dirtyTransform;
};

}
