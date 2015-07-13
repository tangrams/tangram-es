#pragma once

#include "style.h"
#include "style/styleBatch.h"

class PolygonBatch;

class PolygonStyle : public Style {

    struct StyleParams {
        int32_t order = 0;
        uint32_t color = 0xffffffff;
    };

protected:

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual StyleBatch* newBatch() const override;

    auto parseParamMap(const StyleParamMap& _styleParams) const -> StyleParams;

public:

    PolygonStyle(GLenum _drawMode = GL_TRIANGLES);
    PolygonStyle(std::string _name, GLenum _drawMode = GL_TRIANGLES);

    virtual ~PolygonStyle() {}

    friend class PolygonBatch;
};
