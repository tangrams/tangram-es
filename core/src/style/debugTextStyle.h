#pragma once

#include "style/textStyle.h"

namespace Tangram {

class DebugTextStyle : public TextStyle {

public:
    DebugTextStyle(std::string _name, std::shared_ptr<FontContext> _fontContext, bool _sdf = false) : TextStyle(_name, _fontContext, _sdf) {}

    std::unique_ptr<StyleBuilder> createBuilder() const override;

};

}
