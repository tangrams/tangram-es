#pragma once

#include "style/style.h"
#include "style/pointStyle.h"
#include "style/textStyleBuilder.h"
#include <vector>
#include <memory>

namespace Tangram {

struct IconMesh : LabelSet {

    std::unique_ptr<StyledMesh> textLabels;
    std::unique_ptr<StyledMesh> spriteLabels;

    void addLabels(std::vector<std::unique_ptr<Label>>& _labels) {
        typedef std::vector<std::unique_ptr<Label>>::iterator iter_t;
        m_labels.insert(m_labels.end(),
                        std::move_iterator<iter_t>(_labels.begin()),
                        std::move_iterator<iter_t>(_labels.end()));
    }

};

struct PointStyleBuilder : public StyleBuilder {

    const PointStyle& m_style;


    void setup(const Tile& _tile) override;

    bool checkRule(const DrawRule& _rule) const override;

    void addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) override;
    void addLine(const Line& _line, const Properties& _props, const DrawRule& _rule) override;
    void addPoint(const Point& _line, const Properties& _props, const DrawRule& _rule) override;

    std::unique_ptr<StyledMesh> build() override;

    const Style& style() const override { return m_style; }

    PointStyleBuilder(const PointStyle& _style) : StyleBuilder(_style), m_style(_style) {
        m_textStyleBuilder = m_style.textStyle().createBuilder();
    }

    bool getUVQuad(PointStyle::Parameters& _params, glm::vec4& _quad) const;

    PointStyle::Parameters applyRule(const DrawRule& _rule, const Properties& _props) const;

    void addLabel(const Point& _point, const glm::vec4& _quad,
                  const PointStyle::Parameters& _params);

    auto& labels() { return m_labels; }

    virtual void addFeature(const Feature& _feat, const DrawRule& _rule) override;

private:
    std::vector<std::unique_ptr<Label>> m_labels;
    std::vector<SpriteQuad> m_quads;

    std::unique_ptr<IconMesh> iconMesh;

    float m_zoom;
    std::unique_ptr<SpriteLabels> m_spriteLabels;
    std::unique_ptr<StyleBuilder> m_textStyleBuilder;

};

}
