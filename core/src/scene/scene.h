#pragma once

#include <vector>
#include <memory>
#include <map>

#include "style/style.h"
#include "scene/light.h"

/* Singleton container of <Style> information
 *
 * Scene is a singleton containing the styles, lighting, and interactions defining a map scene
 */

class Scene {
public:

    Scene();

    void addStyle(std::unique_ptr<Style> _style);

    /*  Add a Directional Light */
    void addLight(std::unique_ptr<Light> _light);

    std::vector<std::unique_ptr<Style>>& getStyles() { return m_styles; };

    /*  Get all Lights */
    std::vector<std::unique_ptr<Light>>& getLights() { return m_lights; };

private:

    std::vector<std::unique_ptr<Style>> m_styles;
    std::vector<std::unique_ptr<Light>> m_lights;
};

