/*  This is the abstract class that other type of lights can extend from it.
 *  Another possible aproach could be something like the vertexLayout but this one
 *  let you define specific methods for subclassed lights.
 */

#pragma once

#include "glm/glm.hpp"
#include "util/shaderProgram.h"

class AbstractLight {
public:
    
    AbstractLight();
    virtual ~AbstractLight();
    
    /*  This name is used to pass the uniforms to the shaders.
    *   If is on a index pass it other wise is -1 
    *   Ex: [name] = u_[name] 
    */
    void setName(const std::string &_name, int _indexPos = -1);

    /*  Arrays of lights are manage by the scene. And this number is define on the adding lights methods.
    *   Especial lights don't need to be pass on arrays.
    */
    void setIndexPos(int _indexPos);

    /* General light colors */
    void setAmbientColor(const glm::vec4 _ambient);
    void setDiffuseColor(const glm::vec4 _diffuse);
    void setSpecularColor(const glm::vec4 _specular);

    /* Get the name of the light */
    std::string getName();

    /* Get the uniform name of the light */
    std::string getUniformName();

    /*  This method is inspired on the webgl version.
    *   Once we have a better shader injection system we can ask for the code for a specifict light */
    virtual std::string getTransform() = 0;

    /*  This method is inspired on the webgl version.
    *   used to inject the uniforms for this particular light on
    *   the passed shader */
    virtual void setupProgram( ShaderProgram &_shader );
    
protected:

    /* Light Colors */
    glm::vec4 m_ambient;
    glm::vec4 m_diffuse;
    glm::vec4 m_specular;

    /*  The name reference to the uniform on the shader. 
    *  For generic names like "directionalLight", "pointLight" or "spotLight" this will become part of the array:
    * "u_directionalLight[0]", "u_pointLight[0]" or "u_spotLight[0]"  */
    std::string m_name;

    /* If -1 is a single light not injected through the array */
    int         m_index;
};
