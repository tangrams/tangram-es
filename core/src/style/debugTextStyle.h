#pragma once

#include "fontStyle.h"
#include "tangram.h"

class DebugTextStyle : public FontStyle {

protected:
    virtual void addData(TileData& _data, MapTile& _tile, const MapProjection& _mapProjection) const override;

public:

    DebugTextStyle(const std::string& _fontName, std::string _name, float _fontSize, bool _sdf = false, GLenum _drawMode = GL_TRIANGLES);

};
