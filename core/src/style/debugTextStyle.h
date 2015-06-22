#pragma once

#include "textStyle.h"
#include "tangram.h"
#include "typedMesh.h"

class DebugTextStyle : public TextStyle {

protected:

    struct PosTexID {
        glm::vec2 pos;
        glm::vec2 uv;
        float fsID;
    };

    virtual void addData(TileData& _data, MapTile& _tile, const MapProjection& _mapProjection) override;

    typedef TypedMesh<PosTexID> Mesh;

public:

    DebugTextStyle(const std::string& _fontName, std::string _name, float _fontSize, unsigned int _color, bool _sdf = false, GLenum _drawMode = GL_TRIANGLES);

};
