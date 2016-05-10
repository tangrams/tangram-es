#include "iconStyle.h"

#include "style/pointStyleBuilder.h"
#include "style/textStyleBuilder.h"
#include "style/textStyle.h"
#include "scene/styleParam.h"
#include "scene/drawRule.h"

namespace Tangram {

IconStyle::IconStyle(std::string _name, Blending _blendMode, GLenum _drawMode)
    : Style(_name, _blendMode, _drawMode)
{
    m_pointStyle = std::make_unique<PointStyle>(_name, _blendMode, _drawMode);
    m_textStyle = std::make_unique<TextStyle>(_name, true, _blendMode, _drawMode);

    m_textStyle->unified(true);
    m_pointStyle->unified(true);

    TextStyle::Parameters& defaultTextParams = m_textStyle->defaultParams();
    defaultTextParams.anchor = LabelProperty::Anchor::bottom;
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

    virtual void addFeature(const Feature& _feat, const DrawRule& _rule) override;

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
    return true;
}

void IconStyleBuilder::addFeature(const Feature& _feat, const DrawRule& _rule) {
    TextStyleBuilder& tBuilder = static_cast<TextStyleBuilder&>(*textStyleBuilder);
    PointStyleBuilder& pBuilder = static_cast<PointStyleBuilder&>(*pointStyleBuilder);

    tBuilder.addFeature(_feat, _rule);

    if (_feat.geometryType == GeometryType::points) {
        for (auto& point : _feat.points) {
            pBuilder.addPoint(point, _feat.props, _rule);
        }
    } else if (_feat.geometryType == GeometryType::polygons) {
        for (auto& polygon : _feat.polygons) {
            pBuilder.addPolygon(polygon, _feat.props, _rule);
        }
    } else if (_feat.geometryType == GeometryType::lines) {
        for (auto& line : _feat.lines) {
            pBuilder.addLine(line, _feat.props, _rule);
        }
    }

    auto textLabels = tBuilder.labelStack();
    auto pointLabels = pBuilder.labelStack();

    uint32_t textPriority;
    bool definePriority = !_rule.get(StyleParamKey::text_priority, textPriority);

    if (textLabels.size() == pointLabels.size()) {
        for (size_t i = 0; i < textLabels.size(); ++i) {
            auto tLabel = textLabels[i];
            auto pLabel = pointLabels[i];

            // Link labels together
            tLabel->parent(pLabel, definePriority);
        }
    }

    tBuilder.clearLabelStack();
    pBuilder.clearLabelStack();
}

std::unique_ptr<StyledMesh> IconStyleBuilder::build() {
    TextStyleBuilder& tBuilder = static_cast<TextStyleBuilder&>(*textStyleBuilder);
    PointStyleBuilder& pBuilder = static_cast<PointStyleBuilder&>(*pointStyleBuilder);

    auto iconMesh = std::make_unique<IconMesh>();

    iconMesh->spriteLabels = pBuilder.build();
    iconMesh->textLabels = tBuilder.build();

    return std::move(iconMesh);
}

std::unique_ptr<StyleBuilder> IconStyle::createBuilder() const {
    auto iconStyleBuilder = std::make_unique<IconStyleBuilder>(*this);

    iconStyleBuilder->pointStyleBuilder = std::move(m_pointStyle->createBuilder());
    iconStyleBuilder->textStyleBuilder = std::move(m_textStyle->createBuilder());

    return std::move(iconStyleBuilder);
}

}
