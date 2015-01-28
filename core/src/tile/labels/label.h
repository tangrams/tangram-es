#pragma once

#include "glm/glm.hpp"
#include "text/fontContext.h"
#include "text/textBuffer.h"
#include <string>

struct LabelTransform {
    glm::dvec2 m_worldPosition;
    float m_alpha;
    float m_rotation;
};

class Label {

public:

    Label(LabelTransform _transform, std::string _text, std::shared_ptr<TextBuffer> _buffer);
    ~Label();

    void rasterize();

    LabelTransform getTransform() const { return m_transform; }
    void updateTransform(LabelTransform _transform, glm::dmat4 _mvp, glm::dvec2 _screenSize);

private:

    LabelTransform m_transform;
    std::string m_text;
    std::shared_ptr<TextBuffer> m_buffer;
    fsuint m_id;
    
};
