#pragma once

#include "style/style.h"
#include "style/pointStyle.h"
#include <vector>
#include <memory>

namespace Tangram {

struct PointStyleBuilder : public StyleBuilder {

    const PointStyle& m_style;


    void setup(const Tile& _tile) override;

    bool checkRule(const DrawRule& _rule) const override;

    void addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) override;
    void addLine(const Line& _line, const Properties& _props, const DrawRule& _rule) override;
    void addPoint(const Point& _line, const Properties& _props, const DrawRule& _rule) override;

    std::unique_ptr<StyledMesh> build() override;

    const Style& style() const override { return m_style; }

    PointStyleBuilder(const PointStyle& _style) : StyleBuilder(_style), m_style(_style) {}

    bool getUVQuad(PointStyle::Parameters& _params, glm::vec4& _quad) const;

    PointStyle::Parameters applyRule(const DrawRule& _rule, const Properties& _props) const;

    void addLabel(const Point& _point, const glm::vec4& _quad,
                  const PointStyle::Parameters& _params);

    const std::vector<std::shared_ptr<Label>>& labelStack() const { return m_labelStack; }
    void clearLabelStack() { m_labelStack.clear(); }

private:
    std::vector<std::shared_ptr<Label>> m_labels;
    std::vector<std::shared_ptr<Label>> m_labelStack;
    std::vector<SpriteQuad> m_quads;
    
    float m_zoom;
    std::unique_ptr<SpriteLabels> m_spriteLabels;

};

}