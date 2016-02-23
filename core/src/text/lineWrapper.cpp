#include "lineWrapper.h"

namespace Tangram {

alfons::LineMetrics drawWithLineWrapping(const alfons::LineLayout& _line, alfons::TextBatch& _batch,
                                         size_t _maxChar, TextLabelProperty::Align _alignment, float _pixelScale)
{
    static std::vector<std::pair<int,float>> lineWraps;

    lineWraps.clear();

    float lineWidth = 0;
    float maxWidth = 0;
    size_t charCount = 0;
    size_t shapeCount = 0;

    float lastWidth = 0;
    size_t lastShape = 0;
    size_t lastChar = 0;

    for (auto& c : _line.shapes()) {
        shapeCount++;
        lineWidth += _line.advance(c);

        if (c.cluster) { charCount++; }

        if (c.canBreak || c.mustBreak) {
            lastShape = shapeCount;
            lastChar = charCount;
            lastWidth = lineWidth;
        }

        if (lastShape != 0 && (c.mustBreak || charCount >= _maxChar)) {
            // only go to next line if chars have been added on the current line
            // HACK: avoid short words on single line
            if (c.mustBreak || shapeCount - lastShape > 4 || _maxChar <= 4) {
                auto& endShape = _line.shapes()[lastShape-1];

                if (endShape.isSpace) {
                    lineWidth -= _line.advance(endShape);
                    lastWidth -= _line.advance(endShape);
                }

                lineWraps.emplace_back(lastShape, lastWidth);
                maxWidth = std::max(maxWidth, lastWidth);

                lineWidth -= lastWidth;
                charCount -= lastChar;

                lastShape = 0;
            }
        }
    }

    if (charCount > 0) {
        lineWraps.emplace_back(shapeCount, lineWidth);
        maxWidth = std::max(maxWidth, lineWidth);
    }

    size_t shapeStart = 0;
    glm::vec2 position;
    alfons::LineMetrics layoutMetrics;

    for (auto wrap : lineWraps) {
        alfons::LineMetrics lineMetrics;

        switch(_alignment) {
        case TextLabelProperty::Align::center:
            position.x = (maxWidth - wrap.second) * 0.5;
            break;
        case TextLabelProperty::Align::right:
            position.x = (maxWidth - wrap.second);
            break;
        default:
            position.x = 0;
        }

        size_t shapeEnd = wrap.first;

        // Draw line quads
        _batch.draw(_line, shapeStart, shapeEnd, position, lineMetrics);

        shapeStart = shapeEnd;

        float height = lineMetrics.height();
        height -= (2 * 6) * _line.scale(); // substract glyph padding
        height += 4 * _pixelScale; // add some custom line offset

        position.y += height;

        layoutMetrics.addExtents(lineMetrics.aabb);
    }

    return layoutMetrics;
}

}

