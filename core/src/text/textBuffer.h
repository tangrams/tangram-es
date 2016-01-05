#pragma once

#include "labels/labelMesh.h"
#include "style/textStyle.h"
#include "text/fontContext.h"

namespace Tangram {

class TextBuffer {

public:

    // TODO move static Utility functions to some other place
    struct WordBreak {
        int start;
        int end;
    };

    static std::vector<WordBreak> findWords(const std::string& _text);

    static int applyWordWrapping(std::vector<FONSquad>& _quads, const TextStyle::Parameters& _params,
                                 const FontContext::FontMetrics& _metrics, Label::Type _type,
                                 std::vector<WordBreak>& words);

    static std::string applyTextTransform(const TextStyle::Parameters& _params, const std::string& _string);

    TextBuffer() = delete;

};

}
