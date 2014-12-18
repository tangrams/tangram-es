//
//  Based on openGL 2.0 GLSL gl_MaterialParameters
//  http://mew.cx/glsl_quickref.pdf
//
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

    /* Enable Emission colors on the shader */
    void enableEmission();

    /* Enable ambient colors on the shader */
    void enableAmbient();

    /* Enable Diffuse colors on the shader */
    void enableDiffuse();

    /* Enable Specular colors on the shader */
    void enableSpecular();

    /* Get defines that need to be injected on top of the shader */
    virtual std::string getDefinesBlock();

    /* Get the GLSL struct and classes need to be injected */
    static std::string getClassBlock();

    /* Inject both the defines and struct and classes need to be injected on the GLSL shaders */
    virtual std::string getBlock();

    /* Method to pass it self as a uniform to the shader program */
    void        setupProgram( std::shared_ptr<ShaderProgram> _shader );
    
private:
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