#pragma once

#include "glm/glm.hpp"
#include "text/fontContext.h"
#include "text/textBuffer.h"
#include <string>

struct LabelTransform {
    glm::dvec2 m_worldPosition;
    glm::dvec2 m_screenPosition;
    float m_alpha;
    float m_rotation;
};

class Label {

public:

    Label(LabelTransform _transform, std::string _text, std::shared_ptr<FontContext> _fontContext, std::shared_ptr<TextBuffer> _buffer);
    ~Label();

private:

    LabelTransform m_transform;
    std::string m_text;
    std::shared_ptr<FontContext> m_fontContext;
    std::shared_ptr<TextBuffer> m_buffer;
    fsuint m_id;
};
