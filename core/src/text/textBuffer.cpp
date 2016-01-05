#include "textBuffer.h"

#include "labels/textLabel.h"
#include "gl/texture.h"
#include "gl/vboMesh.h"

#include <limits>
#include <memory>
#include <locale>

namespace Tangram {

std::vector<TextBuffer::WordBreak> TextBuffer::findWords(const std::string& _text) {
    std::vector<WordBreak> words;
    unsigned int utf8state = 0;
    const char* str = _text.c_str();
    const char* end = str + strlen(str);
    int wordBegin = 0;
    int wordEnd = -1;
    int itr = 0;

    for (; str != end; ++str) {
        unsigned int byte = *(const unsigned char*)str;

        if (fonsDecUTF8(&utf8state, byte)) {
            continue;
        }

        if (std::isspace(*str)) {
            wordEnd = itr - 1;
            if (wordBegin <= wordEnd) {
                words.push_back({wordBegin, wordEnd});
            }
            if (*str == '\n') {
                words.push_back({itr, itr});
            }
            wordBegin = itr + 1;
        }
        itr++;
    }

    if (!std::isspace(*(end-1))) {
        words.push_back({wordBegin, itr-1});
    }

    return words;
}

int TextBuffer::applyWordWrapping(std::vector<FONSquad>& _quads,
                                  const TextStyle::Parameters& _params,
                                  const FontContext::FontMetrics& _metrics,
                                  Label::Type _type,
                                  std::vector<WordBreak>& _wordBreaks) {
    struct LineQuad {
        std::vector<FONSquad*> quads;
        float length = 0.0f;
    };

    float yOffset = 0.f, xOffset = 0.f;
    int nLine = 1;

    std::vector<LineQuad> lines;
    if (_params.wordWrap < _params.text.length() && _type != Label::Type::line) {
       _wordBreaks = findWords(_params.text);
    }

    lines.push_back(LineQuad()); // atleast one line
    float totalWidth = 0.f;

    // Apply word wrapping based on the word breaks
    for (int iWord = 0; iWord < int(_wordBreaks.size()); iWord++) {
        int start = _wordBreaks[iWord].start;
        int end = _wordBreaks[iWord].end;
        size_t wordSize = end - start + 1;

        auto& lastLineQuads = lines[nLine - 1].quads;

        // Check if quads need to be added to next line?
        if (iWord > 0 && (lastLineQuads.size() + wordSize) > size_t(_params.maxLineWidth)) {
            xOffset = 0.0f;
            auto& quad = _quads[start];
            auto& prevQuad = lines[nLine - 1].quads.front();

            float baseLength = quad.x0 - prevQuad->x0;
            yOffset += _metrics.lineHeight;
            xOffset -= (baseLength);
            lines.push_back(LineQuad());
            nLine++;
        }

        for (int i = start; i <= end; i++) {
            auto& q = _quads[i];

            q.x0 += xOffset;
            q.x1 += xOffset;
            q.y0 += yOffset;
            q.y1 += yOffset;

            lines[nLine - 1].quads.push_back(&q);
            lines[nLine - 1].length = q.x1;

            totalWidth = std::max(totalWidth, q.x1);
        }
    }

    // Apply justification
    for (const auto& line : lines) {
        float paddingRight = totalWidth - line.length;
        float padding = 0;

        switch(_params.align) {
            case TextLabelProperty::Align::left: padding = 0.f; break;
            case TextLabelProperty::Align::right: padding = paddingRight; break;
            case TextLabelProperty::Align::center: padding = paddingRight * 0.5f; break;
        }

        for (auto quad : line.quads) {
            quad->x0 += padding;
            quad->x1 += padding;
        }
    }

    return nLine;
}


std::string TextBuffer::applyTextTransform(const TextStyle::Parameters& _params,
                                           const std::string& _string) {

    std::locale loc;
    std::string text = _string;

    // perfom text transforms
    switch (_params.transform) {
        case TextLabelProperty::Transform::capitalize:
            text[0] = toupper(text[0], loc);
            if (text.size() > 1) {
                for (size_t i = 1; i < text.length(); ++i) {
                    if (text[i - 1] == ' ') {
                        text[i] = std::toupper(text[i], loc);
                    }
                }
            }
            break;
        case TextLabelProperty::Transform::lowercase:
            for (size_t i = 0; i < text.length(); ++i) {
                text[i] = std::tolower(text[i], loc);
            }
            break;
        case TextLabelProperty::Transform::uppercase:
            // TODO : use to wupper when any wide character is detected
            for (size_t i = 0; i < text.length(); ++i) {
                text[i] = std::toupper(text[i], loc);
            }
            break;
        default:
            break;
    }

    return text;
}

}
