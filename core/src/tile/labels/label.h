#pragma once

#include "glm/glm.hpp"
#include "text/fontContext.h"
#include "text/textBuffer.h"
#include <string>

struct LabelTransform {
    glm::vec2 m_worldPosition;
    float m_alpha;
    float m_rotation;
};

class Label {

public:

    Label(LabelTransform _transform, std::string _text, std::shared_ptr<TextBuffer> _buffer);
    ~Label();

    /* Call the font context to rasterize the label string */
    void rasterize();

    LabelTransform getTransform() const { return m_transform; }

    /* Update the transform of the label in world space, and project it to screen space */
    void updateTransform(const LabelTransform& _transform, const glm::mat4& _mvp, const glm::vec2& _screenSize);

private:

    LabelTransform m_transform;
    std::string m_text;
    std::shared_ptr<TextBuffer> m_buffer; // the buffer in which this label text id is associated to
    fsuint m_id;
    
};
