#include "iconStyle.h"

namespace Tangram {

IconStyle::IconStyle(std::string _name, Blending _blendMode, GLenum _drawMode)
    : Style(_name, _blendMode, _drawMode)
{
    m_pointStyle = std::make_unique<PointStyle>(_name, _blendMode, _drawMode);
    m_textStyle = std::make_unique<TextStyle>(_name, true, _blendMode, _drawMode);
}

IconStyle::~IconStyle() {

}

void IconStyle::constructVertexLayout() {

}

void IconStyle::constructShaderProgram() {

}

void IconStyle::onBeginUpdate() {

}

void IconStyle::onBeginFrame() {

}

void IconStyle::onBeginDrawFrame(const View& _view, Scene& _scene) {

}

struct IconStyleBuilder : public StyleBuilder {
    const IconStyle& m_style;

    void setup(const Tile& _tile) override {}
    bool checkRule(const DrawRule& _rule) const override {}

    void addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) override {}
    void addLine(const Line& _line, const Properties& _props, const DrawRule& _rule) override {}
    void addPoint(const Point& _line, const Properties& _props, const DrawRule& _rule) override {}

    // TODO: possibly returns several meshes
    std::unique_ptr<StyledMesh> build() override {
        return nullptr;
    };

    const Style& style() const override { return m_style; }

    IconStyleBuilder(const IconStyle& _style) : StyleBuilder(_style), m_style(_style) {}
};

std::unique_ptr<StyleBuilder> IconStyle::createBuilder() const {
    return std::make_unique<IconStyleBuilder>(*this);
}

}
