#pragma once

#include "alfons/textBatch.h"
#include "alfons/lineLayout.h"
#include "style/labelProperty.h"

namespace Tangram {

struct LineWrap {
    alfons::LineMetrics metrics;
    unsigned int nbLines;
};

/* Wrap an Alfons line layout, and draw the glyph quads to the TextBatch
 *
 * _line the alfons LineLayout containing the glyph shapes and their related codepoints
 * _batch the text batch on which the mesh callback and atlas callback would be triggered
 * _maxChar the maximum line length
 * _minWordLength a parameter to control the minimum word length
 * _alignment align text (center, left, right)
 * _pixelScale the screen pixel density
 */
LineWrap drawWithLineWrapping(const alfons::LineLayout& _line, alfons::TextBatch& _batch,
    size_t _maxChar, size_t _minWordLength, TextLabelProperty::Align _alignment, float _pixelScale);

}

