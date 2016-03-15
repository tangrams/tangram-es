#include "textUtil.h"

#include "platform.h"


namespace Tangram {

LineWrap drawWithLineWrapping(const alfons::LineLayout& _line, alfons::TextBatch& _batch,
                              size_t _minLineChars, size_t _maxLineChars,
                              TextLabelProperty::Align _alignment,  float _pixelScale) {

    static std::vector<std::pair<int,float>> lineWraps;

    lineWraps.clear();

    if (_line.shapes().size() == 0) {
        return {alfons::LineMetrics(), 0};
    }

    float lineWidth = 0;
    float maxWidth = 0;
    size_t charCount = 0;
    size_t shapeCount = 0;

    float lastWidth = 0;
    size_t lastShape = 0;
    size_t lastChar = 0;

    for (auto& shape : _line.shapes()) {

        if (!shape.cluster) {
            shapeCount++;
            lineWidth += _line.advance(shape);
            continue;
        }

        charCount++;
        shapeCount++;
        lineWidth += _line.advance(shape);

        if (shape.canBreak || shape.mustBreak) {
            lastShape = shapeCount;
            lastChar = charCount;
            lastWidth = lineWidth;
        }

        if (lastShape != 0 && (shape.mustBreak || charCount >= _maxLineChars)) {
            // only go to next line if chars have been added on the current line
            if (shape.mustBreak || charCount - lastChar >= _minLineChars) {
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
        _batch.drawShapeRange(_line, shapeStart, shapeEnd, position, lineMetrics);

        shapeStart = shapeEnd;

        // FIXME hardcoded values
        float height = lineMetrics.height();
        height -= (2 * 6) * _line.scale(); // substract glyph padding
        height += 4 * _pixelScale; // add some custom line offset

        position.y += height;

        layoutMetrics.addExtents(lineMetrics.aabb);
    }

    return {layoutMetrics, int(lineWraps.size())};
}


}
