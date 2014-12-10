#pragma once

#include <vector>
#include <memory>

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
    
    /* get all Lights */
    std::vector<std::unique_ptr<Light>>& getLights(){ return m_lights; };

    /*  Once all lights are added to the scene this methods inject the glsl code for them 
    *   as long the have a "#pragma tangram: lighting" reference on the .vs and .fs shaders */
    void injectLightning();

private:

    std::vector<std::unique_ptr<Style>> m_styles;
    std::vector<std::unique_ptr<Light>> m_lights;

    int m_directionaLightCounter;
    int m_pointLightCounter;
    int m_spotLightCounter;
};

