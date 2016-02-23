#pragma once

#include "alfons/textBatch.h"
#include "alfons/lineLayout.h"
#include "style/labelProperty.h"

namespace Tangram {

struct LineWrap {
    alfons::LineMetrics metrics;
    unsigned int nbLines;
};

LineWrap drawWithLineWrapping(const alfons::LineLayout& _line, alfons::TextBatch& _batch,
    size_t _maxChar, TextLabelProperty::Align _alignment, float _pixelScale);

}

