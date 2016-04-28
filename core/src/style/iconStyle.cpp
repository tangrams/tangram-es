#include "iconStyle.h"

#include "style/pointStyleBuilder.h"
#include "style/textStyleBuilder.h"

namespace Tangram {

IconStyle::IconStyle(std::string _name, Blending _blendMode, GLenum _drawMode)
    : Style(_name, _blendMode, _drawMode)
{
    m_pointStyle = std::make_unique<PointStyle>(_name, _blendMode, _drawMode);
    m_textStyle = std::make_unique<TextStyle>(_name, true, _blendMode, _drawMode);
}

IconStyle::~IconStyle() {}

void IconStyle::constructVertexLayout() {
    m_pointStyle->constructVertexLayout();
    m_textStyle->constructVertexLayout();
}

void IconStyle::constructShaderProgram() {
    m_pointStyle->constructShaderProgram();
    m_textStyle->constructShaderProgram();
}

void IconStyle::onBeginUpdate() {
    m_pointStyle->onBeginUpdate();
    m_textStyle->onBeginUpdate();
}

void IconStyle::onBeginFrame() {
    m_pointStyle->onBeginFrame();
    m_textStyle->onBeginFrame();
}

void IconStyle::onBeginDrawFrame(const View& _view, Scene& _scene) {
    m_pointStyle->onBeginDrawFrame(_view, _scene);
    m_textStyle->onBeginDrawFrame(_view, _scene);
}

struct IconStyleBuilder : public StyleBuilder {

    void setup(const Tile& _tile) override;
    bool checkRule(const DrawRule& _rule) const override;

    void addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) override;
    void addLine(const Line& _line, const Properties& _props, const DrawRule& _rule) override;
    void addPoint(const Point& _line, const Properties& _props, const DrawRule& _rule) override;

    // TODO: possibly returns several meshes
    std::unique_ptr<StyledMesh> build() override;

    const Style& style() const override { return m_style; }

    IconStyleBuilder(const IconStyle& _style) : StyleBuilder(_style), m_style(_style) {}

    std::unique_ptr<StyleBuilder> pointStyleBuilder;
    std::unique_ptr<StyleBuilder> textStyleBuilder;

private:
    const IconStyle& m_style;
};

void IconStyleBuilder::setup(const Tile& _tile) {
    TextStyleBuilder& tBuilder = static_cast<TextStyleBuilder&>(*textStyleBuilder);
    PointStyleBuilder& pBuilder = static_cast<PointStyleBuilder&>(*pointStyleBuilder);

    tBuilder.setup(_tile);
    pBuilder.setup(_tile);
}

bool IconStyleBuilder::checkRule(const DrawRule& _rule) const {
    return false;
}

void IconStyleBuilder::addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) {

}

void IconStyleBuilder::addLine(const Line& _line, const Properties& _props, const DrawRule& _rule) {

}

void IconStyleBuilder::addPoint(const Point& _line, const Properties& _props, const DrawRule& _rule) {

}

std::unique_ptr<StyledMesh> IconStyleBuilder::build() {
    return nullptr;
};


std::unique_ptr<StyleBuilder> IconStyle::createBuilder() const {
    auto iconStyleBuilder = std::make_unique<IconStyleBuilder>(*this);

    iconStyleBuilder->pointStyleBuilder = std::move(m_pointStyle->createBuilder());
    iconStyleBuilder->textStyleBuilder = std::move(m_textStyle->createBuilder());

    return std::move(iconStyleBuilder);
}

}
