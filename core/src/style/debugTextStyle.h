#pragma once

#include "textStyle.h"
#include "tangram.h"

class DebugTextStyle : public TextStyle {

protected:

    virtual void addData(TileData& _data, MapTile& _tile) override;

public:

    DebugTextStyle(const std::string& _fontName, std::string _name, float _fontSize, unsigned int _color, bool _sdf = false, GLenum _drawMode = GL_TRIANGLES);

};
