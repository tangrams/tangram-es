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
    
    void addStyle(std::unique_ptr<Style> _style);
    
    void addDirectionalLight(std::unique_ptr<DirectionalLight> _dLight);
    void addPointLight(std::unique_ptr<PointLight> _pLight);
    void addSpotLight(std::unique_ptr<SpotLight> _sLight);

    std::vector<std::unique_ptr<Style>>& getStyles() { return m_styles; };
    
    std::vector<std::unique_ptr<DirectionalLight>>& getDirectionalLights() { return m_directionalLights; };
    std::vector<std::unique_ptr<PointLight>>& getPointLights() { return m_pointLights; };
    std::vector<std::unique_ptr<SpotLight>>& getSpotLights() { return m_spotLights; };

private:

    std::vector<std::unique_ptr<Style>> m_styles;
    
    std::vector<std::unique_ptr<DirectionalLight>> m_directionalLights;
    std::vector<std::unique_ptr<PointLight>> m_pointLights;
    std::vector<std::unique_ptr<SpotLight>> m_spotLights;
};

