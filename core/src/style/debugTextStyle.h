#pragma once

#include "textStyle.h"
#include "tangram.h"

namespace Tangram {

class DebugTextStyle : public TextStyle {

protected:

    virtual void onBeginBuildTile(Tile& _tile) const override;

public:

    DebugTextStyle(std::shared_ptr<FontContext> _fontContext, FontID _fontId, std::string _name,
                   float _fontSize, bool _sdf = false, bool _sdfMultisampling = false,
                   Blending _blendMode = Blending::overlay, GLenum _drawMode = GL_TRIANGLES);

    FontID m_font;
    float m_fontSize;
};

}
