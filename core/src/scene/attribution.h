#pragma once

#include <memory>

namespace Tangram {

class Marker;
class RenderState;
class StyleBuilder;
class Scene;
class TextStyle;
class View;

class Attribution {
    std::unique_ptr<TextStyle> m_style = nullptr;
    std::unique_ptr<StyleBuilder> m_builder = nullptr;
    std::unique_ptr<Marker> m_marker = nullptr;

public:
    void reset();
    void draw(RenderState& rs, const View& view, Scene& scene);
    void update(const View& view);
    void setup(Scene& scene, const std::string& attributionText, const std::string& attributionStyling);
    Marker& marker() { return *m_marker; }
    const std::unique_ptr<TextStyle>& style() { return m_style; }
    StyleBuilder* builder() { return m_builder.get(); }
    bool isSetup() { return (m_style && m_builder && m_marker); }
};

} // namespace Tangram
