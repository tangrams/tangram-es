#pragma once

#include "labels/labelProperty.h"

#include "alfons/alfons.h"
#include "alfons/lineLayout.h"
#include "alfons/textBatch.h"
#include <vector>

namespace Tangram {

class TextWrapper {

public:

    float getShapeRangeWidth(const alfons::LineLayout& _line,
        size_t _minLineChars, size_t _maxLineChars);

    void clearWraps();

    /* Wrap an Alfons line layout, and draw the glyph quads to the TextBatch.
     *
     * This method is not threadsafe!
     *
     * _batch the text batch on which the mesh callback and atlas callback would be triggered
     * _line the alfons LineLayout containing the glyph shapes and their related codepoints
     * _maxChar the maximum line length
     * _minWordLength a parameter to control the minimum word length
     * _alignment align text (center, left, right)
     * _lineSpacing
     * _metrics out: text extents
     */
    int draw(alfons::TextBatch& _batch, float _maxWidth, const alfons::LineLayout& _line,
             TextLabelProperty::Align _alignment, float _lineSpacing,
             alfons::LineMetrics& _metrics);

private:
    std::vector<std::pair<int,float>> m_lineWraps;
};

}
