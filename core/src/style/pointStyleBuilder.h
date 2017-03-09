#pragma once

#include "style/style.h"
#include "style/pointStyle.h"
#include "style/textStyleBuilder.h"
#include <map>
#include <memory>
#include <vector>

namespace Tangram {

struct IconMesh : LabelSet {

    std::unique_ptr<StyledMesh> textLabels;
    std::unique_ptr<StyledMesh> spriteLabels;

    void setTextLabels(std::unique_ptr<StyledMesh> _textLabels);
};

struct PointStyleBuilder : public StyleBuilder {

    const PointStyle& m_style;


    void setup(const Tile& _tile) override;
    void setup(const Marker& _marker, int zoom) override;

    bool checkRule(const DrawRule& _rule) const override;

    bool addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) override;
    bool addLine(const Line& _line, const Properties& _props, const DrawRule& _rule) override;
    bool addPoint(const Point& _line, const Properties& _props, const DrawRule& _rule) override;

    std::unique_ptr<StyledMesh> build() override;

    const Style& style() const override { return m_style; }

    PointStyleBuilder(const PointStyle& _style) : m_style(_style) {
        m_textStyleBuilder = m_style.textStyle().createBuilder();
    }

    bool getUVQuad(PointStyle::Parameters& _params, glm::vec4& _quad) const;

    PointStyle::Parameters applyRule(const DrawRule& _rule, const Properties& _props) const;

    // Gets points for label placement and appropriate angle for each label (if `auto` angle is set)
    void labelPointsPlacing(const Line& _line, const glm::vec4& _quad,
                            PointStyle::Parameters& _params, const DrawRule& _rule);

    void addLabel(const Point& _point, const glm::vec4& _quad,
                  const PointStyle::Parameters& _params, const DrawRule& _rule);

    void addLayoutItems(LabelCollider& _layout) override;

    bool addFeature(const Feature& _feat, const DrawRule& _rule) override;

private:


    std::vector<std::unique_ptr<Label>> m_labels;
    std::vector<SpriteQuad> m_quads;

    std::unique_ptr<IconMesh> m_iconMesh;

    float m_zoom = 0;
    float m_styleZoom = 0;
    float m_tileScale = 1;
    std::unique_ptr<SpriteLabels> m_spriteLabels;
    std::unique_ptr<StyleBuilder> m_textStyleBuilder;

    // Non-owning reference to a texture to use for the current feature.
    Texture* m_texture = nullptr;

};

}
