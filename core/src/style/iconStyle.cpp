#include "iconStyle.h"

#include "style/pointStyleBuilder.h"
#include "style/textStyleBuilder.h"
#include "style/textStyle.h"
#include "scene/styleParam.h"
#include "scene/drawRule.h"
#include "labels/textLabels.h"

namespace Tangram {

IconStyle::IconStyle(std::string _name, Blending _blendMode, GLenum _drawMode)
    : Style(_name, _blendMode, _drawMode) {

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

void IconStyle::setPixelScale(float _pixelScale) {
    m_pointStyle->setPixelScale(_pixelScale);
    m_textStyle->setPixelScale(_pixelScale);
}

struct IconStyleBuilder : public StyleBuilder {

    void setup(const Tile& _tile) override;
    bool checkRule(const DrawRule& _rule) const override;

    virtual void addFeature(const Feature& _feat, const DrawRule& _rule) override;

    std::unique_ptr<StyledMesh> build() override;

    const Style& style() const override { return m_style; }

    IconStyleBuilder(const IconStyle& _style) : StyleBuilder(_style), m_style(_style) {}

    std::unique_ptr<PointStyleBuilder> pointStyleBuilder;
    std::unique_ptr<TextStyleBuilder> textStyleBuilder;

private:
    const IconStyle& m_style;
};

void IconStyleBuilder::setup(const Tile& _tile) {
    textStyleBuilder->setup(_tile);
    pointStyleBuilder->setup(_tile);
}

bool IconStyleBuilder::checkRule(const DrawRule& _rule) const {
    return true;
}

void IconStyleBuilder::addFeature(const Feature& _feat, const DrawRule& _rule) {

    auto& textLabels = textStyleBuilder->labels();
    auto& pointLabels = pointStyleBuilder->labels();

    size_t textLabelStart = textLabels.size();
    size_t pointLabelStart = pointLabels.size();

    if (_feat.geometryType == GeometryType::points) {
        for (auto& point : _feat.points) {
            pointStyleBuilder->addPoint(point, _feat.props, _rule);
        }
    } else if (_feat.geometryType == GeometryType::polygons) {
        for (auto& polygon : _feat.polygons) {
            pointStyleBuilder->addPolygon(polygon, _feat.props, _rule);
        }
    } else if (_feat.geometryType == GeometryType::lines) {
        for (auto& line : _feat.lines) {
            pointStyleBuilder->addLine(line, _feat.props, _rule);
        }
    }

    size_t pointLabelCount = pointLabels.size() - pointLabelStart;

    if (pointLabelCount == 0) { return; }

    textStyleBuilder->addFeature(_feat, _rule);

    size_t textLabelCount = textLabels.size() - textLabelStart;

    uint32_t textPriority;
    bool definePriority = !_rule.get(StyleParamKey::text_priority, textPriority);

    if (textLabelCount == pointLabelCount) {
        for (size_t i = 0; i < textLabelCount; ++i) {
            auto& tLabel = textLabels[textLabelStart + i];
            auto& pLabel = pointLabels[pointLabelStart + i];

            // Link labels together
            tLabel->setParent(*pLabel, definePriority);
        }
    } else if (textLabelCount > 0) {
        // TODO remove unused text labels
        // textLabels.popLabels(textLabelCount);
    }
}

std::unique_ptr<StyledMesh> IconStyleBuilder::build() {

    auto iconMesh = std::make_unique<IconMesh>();

    iconMesh->spriteLabels = pointStyleBuilder->build();
    iconMesh->textLabels = textStyleBuilder->build();

    return std::move(iconMesh);
}

std::unique_ptr<StyleBuilder> IconStyle::createBuilder() const {
    auto iconStyleBuilder = std::make_unique<IconStyleBuilder>(*this);

    auto *pBuilder = static_cast<PointStyleBuilder*>(m_pointStyle->createBuilder().release());
    iconStyleBuilder->pointStyleBuilder.reset(pBuilder);

    auto *tBuilder = static_cast<TextStyleBuilder*>(m_textStyle->createBuilder().release());
    iconStyleBuilder->textStyleBuilder.reset(tBuilder);

    return std::move(iconStyleBuilder);
}

}
