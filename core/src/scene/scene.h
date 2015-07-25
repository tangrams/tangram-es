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

    Scene();

    void addStyle(std::unique_ptr<Style> _style);
    void addLight(std::unique_ptr<Light> _light);

    std::vector<std::unique_ptr<Style>>& getStyles() { return m_styles; };
    std::vector<std::unique_ptr<Light>>& getLights() { return m_lights; };

    std::unordered_map<std::string, std::shared_ptr<Texture>>& getTextures() { return m_textures; };

private:

    std::vector<std::unique_ptr<Style>> m_styles;
    std::vector<std::unique_ptr<Light>> m_lights;

    std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;

};

}
