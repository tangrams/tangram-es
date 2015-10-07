#pragma once

#include "style.h"

namespace Tangram {

class DebugStyle : public Style {

protected:

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void onBeginBuildTile(Tile& _tile) const override;

    virtual VboMesh* newMesh() const override {
        return nullptr;
    };

public:

    DebugStyle(std::string _name, Blending _blendMode = Blending::overlay, GLenum _drawMode = GL_LINE_LOOP);

    virtual ~DebugStyle() {
    }

};

}
