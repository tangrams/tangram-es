/*  This is the abstract class that other type of lights can extend from it.
 *  Another possible aproach could be something like the vertexLayout but this one
 *  let you define specific methods for subclassed lights.
 */

#pragma once
 
#include <memory>
#include "glm/glm.hpp"
#include "util/shaderProgram.h"

enum class LightType {
    LIGHT_NOT_DEFINE,
    LIGHT_DIRECTIONAL,
    LIGHT_POINT,
    LIGHT_SPOT,
    LIGHT_CUSTOM
};

class Light {
public:
    
    /* All lights have a name*/
    Light(const std::string& _name, bool _dynamic = false);

    virtual ~Light();
    
    /*  This name is used to construct the uniform name to be pass to the shader */
    virtual void setName(const std::string &_name);

    /*  Set Ambient Color. Which is constant across the scene */
    virtual void setAmbientColor(const glm::vec4 _ambient);

    /*  Set Diffuse Color. What we generaly understand for color of a light */
    virtual void setDiffuseColor(const glm::vec4 _diffuse);

    /*  Set Specular Color. This are the intense reflections of a light. AKA shinny spot */
    virtual void setSpecularColor(const glm::vec4 _specular);

    /*  Get the type of light, especially to identify the class and specific methods to it. */
    virtual LightType getType();

    /*  Get the name of the light */
    virtual std::string getName();

    /*  Get the uniform name of the DYNAMICAL light */
    virtual std::string getUniformName();

    /*  Get the instances light name defined on the shader */
    virtual std::string getInstanceName();

    /*  Get the instances GLSL block where the light is defined inside the shader */
    virtual std::string getInstanceBlock();

    /*  Get the instances GLSL block where NON DYNAMICAL light values are assigned inside the shader */
    virtual std::string getInstanceAssignBlock();

    /*  GLSL #defines flags for the instance of this light */
    virtual std::string getInstanceDefinesBlock() = 0;

    /*  GLSL line to compute the specific light instance */
    virtual std::string getInstanceComputeBlock();

    /*  Inject the uniforms for this particular DYNAMICAL light on the passed shader */
    virtual void setupProgram( std::shared_ptr<ShaderProgram> _shader );

protected:

    /*  The name reference to the uniform on the shader.  */
    std::string m_name;

    /*  String with the type name */
    std::string m_typeName;

    /* Light Colors */
    glm::vec4 m_ambient;
    glm::vec4 m_diffuse;
    glm::vec4 m_specular;

    /*  This is use to identify the type of light after been pull inside a vector of uniq_ptr of this abstract class*/
    LightType   m_type;

    bool        m_dynamic;
};
