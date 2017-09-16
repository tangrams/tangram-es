#include "text/textUtil.h"

#include "platform.h"

namespace Tangram {

float TextWrapper::getShapeRangeWidth(const alfons::LineLayout& _line) {
    float maxWidth = 0;

    float lineWidth = 0;
    size_t charCount = 0;
    size_t shapeCount = 0;

    float lastWidth = 0;
    size_t lastShape = 0;
    size_t lastChar = 0;

    for (auto it = _line.shapes().begin(), end = _line.shapes().end(); it != end; it++) {

        auto &shape = *it;

        charCount++;
        shapeCount++;
        lineWidth += _line.advance(shape);

        if (shape.mustBreak) {
            lastWidth = lineWidth;

            // Append shapes of current glyph cluster
            while ((it+1 != end) && !(it+1)->cluster) {
                it++;
                lineWidth += _line.advance(*it);
                shapeCount++;
            }

            lastChar = charCount;
            lastShape = shapeCount;

            auto& endShape = _line.shapes()[lastShape-1];

            if (endShape.isSpace) {
                lineWidth -= _line.advance(endShape);
                lastWidth -= _line.advance(endShape);
            }

            m_lineWraps.emplace_back(lastShape, lastWidth);
            maxWidth = std::max(maxWidth, lastWidth);

            lineWidth -= lastWidth;
            charCount -= lastChar;
            lastShape = 0;
        }
    }

    if (charCount > 0) {
        m_lineWraps.emplace_back(shapeCount, lineWidth);
        maxWidth = std::max(maxWidth, lineWidth);
    }

    return maxWidth;
}

void TextWrapper::clearWraps() {
    m_lineWraps.clear();
}

int TextWrapper::draw(alfons::TextBatch& _batch, float _maxWidth, const alfons::LineLayout& _line,
                      TextLabelProperty::Align _alignment, float _lineSpacing,
                      alfons::LineMetrics& _layoutMetrics) {
    size_t shapeStart = 0;
    glm::vec2 position;

    for (auto wrap : m_lineWraps) {
        alfons::LineMetrics lineMetrics;

        switch(_alignment) {
            case TextLabelProperty::Align::center:
            case TextLabelProperty::Align::none:
                position.x = (_maxWidth - wrap.second) * 0.5;
                break;
            case TextLabelProperty::Align::right:
                position.x = (_maxWidth - wrap.second);
                break;
            default:
                position.x = 0;
        }

        size_t shapeEnd = wrap.first;

        // Draw line quads
        _batch.drawShapeRange(_line, shapeStart, shapeEnd, position, lineMetrics);

        shapeStart = shapeEnd;

        // FIXME hardcoded value for SDF radius 6
        float height = lineMetrics.height();
        height -= (2 * 6) * _line.scale(); // substract glyph padding
        height += _lineSpacing; // add some custom line offset

        position.y += height;

        _layoutMetrics.addExtents(lineMetrics.aabb);
    }

    return int(m_lineWraps.size());
}


}
