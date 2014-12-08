#pragma once

#include <vector>
#include <memory>

#include "style/style.h"
#include "scene/lights.h"

/* Singleton container of <Style> information
 *
 * Scene is a singleton containing the styles, lighting, and interactions defining a map scene 
 */
class Scene {
public:

    void addStyle(std::unique_ptr<Style> _style);
    
    /*  Add a Directional Light */
    void addLight(std::unique_ptr<DirectionalLight> _light);

    /*  Add a Point Light */
    void addLight(std::unique_ptr<PointLight> _light);

    /*  Add a Spot Light*/
    void addLight(std::unique_ptr<SpotLight> _light);

    std::vector<std::unique_ptr<Style>>& getStyles() { return m_styles; };
    
    /* get all Directional Lights */
    std::vector<std::unique_ptr<DirectionalLight>>& getDirectionalLights() { return m_directionalLights; };

    /* get all Point Lights */
    std::vector<std::unique_ptr<PointLight>>& getPointLights() { return m_pointLights; };

    /* get all Spot Lights */
    std::vector<std::unique_ptr<SpotLight>>& getSpotLights() { return m_spotLights; };

    /*  Once all lights are added to the scene this methods inject the glsl code for them 
    *   as long the have a "#pragma tangram: lighting" reference on the .vs and .fs shaders */
    void injectLightning();

private:

    std::vector<std::unique_ptr<Style>> m_styles;
    
    std::vector<std::unique_ptr<DirectionalLight>> m_directionalLights;
    std::vector<std::unique_ptr<PointLight>> m_pointLights;
    std::vector<std::unique_ptr<SpotLight>> m_spotLights;
};

