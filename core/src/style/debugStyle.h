#pragma once

#include "style.h"
#include "typedMesh.h"

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
    virtual void addData(TileData& _data, MapTile& _tile) override;

    typedef TypedMesh<PosColVertex> Mesh;

    virtual Batch* newBatch() const override {
        return nullptr;
    };

public:

    DebugStyle(GLenum _drawMode = GL_LINE_LOOP);
    DebugStyle(std::string _name, GLenum _drawMode = GL_LINE_LOOP);

    virtual ~DebugStyle() {
    }

};
