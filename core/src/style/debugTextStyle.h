#pragma once

#include "textStyle.h"
#include "tangram.h"
#include "typedMesh.h"

class DebugTextStyle : public TextStyle {

protected:

    virtual void addData(TileData& _data, MapTile& _tile, const MapProjection& _mapProjection) const override;

public:

    DebugTextStyle(const std::string& _fontName, std::string _name, float _fontSize, unsigned int _color, bool _sdf = false, GLenum _drawMode = GL_TRIANGLES);

};
