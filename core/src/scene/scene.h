#pragma once

#include "scene/light.h"
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

    auto& styles() { return m_styles; };
    auto& lights() { return m_lights; };
    auto& textures() { return m_textures; };

    std::shared_ptr<Style> findStyle(const std::string& _name);
    std::shared_ptr<Light> findLight(const std::string& _name);

private:

    std::vector<std::shared_ptr<Style>> m_styles;
    std::vector<std::shared_ptr<Light>> m_lights;
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;

};

}
