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
    std::map<std::string, std::unique_ptr<Light>>& getLights(){ return m_lights; };

    /*  Once all lights are added to the scene this methods inject the glsl code for them 
    *   as long the have a "#pragma tangram: lighting" reference on the .vs and .fs shaders */
    void buildShaders();

private:

    std::vector<std::unique_ptr<Style>> m_styles;
    std::map<std::string, std::unique_ptr<Light>> m_lights;
};

