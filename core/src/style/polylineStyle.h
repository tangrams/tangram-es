#pragma once

#include "style.h"

// for cap and join styles...
#include "util/builders.h"

class PolylineBatch;

class PolylineStyle : public Style {

    struct StyleParams {
        int32_t order = 0;
        uint32_t color = 0xffffffff;
        float width = 1.f;
        CapTypes cap = CapTypes::butt;
        JoinTypes join = JoinTypes::miter;
        float outlineWidth = 1.f;
        uint32_t outlineColor = 0xffffffff;
        bool outlineOn = false;
        CapTypes outlineCap = CapTypes::butt;
        JoinTypes outlineJoin = JoinTypes::miter;
    };

protected:
    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual StyleBatch* newBatch() const override;

    auto parseParamMap(const StyleParamMap& _styleParams) const -> StyleParams;

public:

    PolylineStyle(GLenum _drawMode = GL_TRIANGLES);
    PolylineStyle(std::string _name, GLenum _drawMode = GL_TRIANGLES);

    virtual ~PolylineStyle() {}

    friend class PolylineBatch;
};
