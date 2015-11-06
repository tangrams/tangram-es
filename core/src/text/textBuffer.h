#pragma once

#include "gl.h"
#include "labels/labelMesh.h"
#include "style/textStyle.h"
#include "text/fontContext.h"

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

    struct WordBreak {
        int start;
        int end;
    };

    static std::vector<WordBreak> findWords(const std::string& _text);

private:
    static int applyWordWrapping(std::vector<FONSquad>& _quads, const TextStyle::Parameters& _params,
                                 const FontContext::FontMetrics& _metrics, Label::Type _type,
                                 glm::vec2* _bbox, std::vector<TextBuffer::WordBreak>& words);

    static std::string applyTextTransform(const TextStyle::Parameters& _params, const std::string& _string);

};

}
