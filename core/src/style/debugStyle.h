#pragma once

#include "style.h"
#include "gl/typedMesh.h"

namespace Tangram {

class DebugStyle : public Style {

protected:

    struct PosColVertex {
        // Position Data
        glm::vec3 pos;
        // Color Data
        GLuint abgr;
    };

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void onBeginBuildTile(Tile& _tile) const override;

    typedef TypedMesh<PosColVertex> Mesh;

    virtual VboMesh* newMesh() const override {
        return nullptr;
    };

public:

    DebugStyle(std::string _name, Blending _blendMode = Blending::overlay, GLenum _drawMode = GL_LINE_LOOP);

    virtual ~DebugStyle() {
    }

};

}
