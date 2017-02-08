#pragma once

#include "style.h"

namespace Tangram {

class GridStyle : public Style {

public:

    GridStyle(std::string _name, uint32_t _resolution, Blending _blendMode = Blending::opaque, bool _selection = true);

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual std::unique_ptr<StyleBuilder> createBuilder() const override;
    virtual ~GridStyle() {}

    uint32_t resolution() const { return m_resolution; }

protected:

    uint32_t m_resolution;

};

}
