#pragma once

#include "scene/light.h"
#include "scene/dataLayer.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace Tangram {

class Style;
class Texture;

/* Singleton container of <Style> information
 *
 * Scene is a singleton containing the styles, lighting, and interactions defining a map scene
 */

class Scene {
public:
    Scene();
    ~Scene();

    auto& layers() { return m_layers; };
    auto& styles() { return m_styles; };
    auto& lights() { return m_lights; };
    auto& textures() { return m_textures; };

    const auto& layers() const { return m_layers; };
    const auto& styles() const { return m_styles; };
    const auto& lights() const { return m_lights; };

    const Style* findStyle(const std::string& _name) const;
    const Light* findLight(const std::string& _name) const;

private:

    std::vector<DataLayer> m_layers;
    std::vector<std::unique_ptr<Style>> m_styles;
    std::vector<std::unique_ptr<Light>> m_lights;
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;

};

}
