/* MATERIAL 
-------------------------------
This openGL Material implementation mimics the behavior of 
the WebGL version of Tangram ( https://github.com/tangrams/tangram/wiki/Materials-Overview )
*/

#pragma once
 
#include <memory>
#include "glm/glm.hpp"
#include "util/shaderProgram.h"

class Material {
public:
    
    Material();
    
    virtual ~Material(){};

    /*  Emission color is by default disblable and vec4(0.0).
    *   By setting the property you will enable and require to reloading the shader. */
    void setEmission(const glm::vec4 _emission);

    /*  Ambient color is by default disblable and vec4(1.0).
    *   By setting the property you will enable and require to reloading the shader. */
    void setAmbient(const glm::vec4 _ambient);

    /*  Difuse color is by default enable and vec4(1.0).
    *   If you want to disable it you have to disableDiffuse and reloading the shader. */
    void setDiffuse(const glm::vec4 _diffuse);

    /*  Specular color is by default disblable and vec4(0.2) with a shinny factor of 0.2.
    *   By setting the property you will enable and require to reloading the shader. */
    void setSpecular(const glm::vec4 _specular, float _shinnyFactor);

    /* Enable or disable emission colors */
    void setEmissionEnabled(bool _enable);

    /* Enable or disable ambient colors */
    void setAmbientEnabled(bool _enable);

    /* Enable or disable diffuse colors */
    void setDiffuseEnabled(bool _enable);

    /* Enable or disable specular colors */
    void setSpecularEnabled(bool _enable);

    /*  Inject the needed lines of GLSL code on the shader to make this material work */
    virtual void injectOnProgram( std::shared_ptr<ShaderProgram> _shader );

    /*  Method to pass it self as a uniform to the shader program */
    virtual void setupProgram( std::shared_ptr<ShaderProgram> _shader );
    
private:

    /* Get defines that need to be injected on top of the shader */
    virtual std::string getDefinesBlock();

    /* Get the GLSL struct and classes need to be injected */
    virtual std::string getClassBlock();

    std::string m_name;
    
    glm::vec4   m_emission;
    glm::vec4   m_ambient;
    glm::vec4   m_diffuse;
    glm::vec4   m_specular;
    
    float       m_shininess;

    bool        m_bEmission;
    bool        m_bAmbient;
    bool        m_bDiffuse;
    bool        m_bSpecular;
};