#pragma once

#include "gl.h"
#include "labels/labelMesh.h"
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

private:

    struct WordBreak {
        int start;
        int end;
    };

    std::vector<WordBreak> findWordBreaks(const std::string& _text);

};

}
