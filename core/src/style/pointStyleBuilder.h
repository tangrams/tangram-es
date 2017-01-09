#pragma once

#include "style/style.h"
#include "style/pointStyle.h"
#include "style/textStyleBuilder.h"
#include <vector>
#include <memory>
#include <map>

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

    PointStyleBuilder(const PointStyle& _style) : StyleBuilder(_style), m_style(_style) {
        m_textStyleBuilder = m_style.textStyle().createBuilder();
    }

    bool getUVQuad(PointStyle::Parameters& _params, glm::vec4& _quad) const;

    PointStyle::Parameters applyRule(const DrawRule& _rule, const Properties& _props) const;

    // Gets points for label placement and appropriate angle for each label (if `auto` angle is set)
    void labelPointsPlacing(const Line& _line, const PointStyle::Parameters& params);

    void addLabel(const Point& _point, const glm::vec4& _quad,
                  const PointStyle::Parameters& _params, const DrawRule& _rule);

    void addLayoutItems(LabelCollider& _layout) override;

    bool addFeature(const Feature& _feat, const DrawRule& _rule) override;

private:


    Point interpolateSegment(const Line& _line, int i, int j, float distance);
    Point interpolateLine(const Line& _line, float distance, float minLineLength,
                          const PointStyle::Parameters& params,
                          std::vector<float>& angles);

    std::vector<std::unique_ptr<Label>> m_labels;
    std::vector<SpriteQuad> m_quads;

    std::unique_ptr<IconMesh> m_iconMesh;

    float m_zoom = 0;
    std::unique_ptr<SpriteLabels> m_spriteLabels;
    std::unique_ptr<StyleBuilder> m_textStyleBuilder;

    // Store the calculated point placements to be passed to the child text style builder
    Line m_placedPoints;

    // Store the angles for the placed points, when scene file has an "auto" set for angle.
    std::vector<float> m_angleValues;

    // cache point distances instead of calculating these again and again (sqrt is expensive)
    std::map<std::pair<int, int>, float> m_pointDistances;

    // Non-owning reference to a texture to use for the current feature.
    Texture* m_texture = nullptr;

};

}
