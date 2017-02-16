#pragma once

#include "gl/uniform.h"

#include "glm/vec4.hpp"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Tangram {

class RenderState;
class ShaderProgram;
class View;

enum class LightType {
    ambient,
    directional,
    point,
    spot
};

enum class LightOrigin {
    camera,
    ground,
    world
};

static inline std::string lightOriginString(LightOrigin origin) {
    switch(origin) {
        case LightOrigin::camera: return "camera";
        case LightOrigin::ground: return "ground";
        case LightOrigin::world: return "world";
        default: return "undefined";
    }
}

struct LightUniforms {
    LightUniforms(const std::string& _name)
        : ambient(_name+".ambient"),
          diffuse(_name+".diffuse"),
          specular(_name+".specular") {}

    virtual ~LightUniforms() {}

    UniformLocation ambient;
    UniformLocation diffuse;
    UniformLocation specular;
};

/*  This is the abstract class that other type of lights can extend from it.
 *  Another possible aproach could be something like the vertexLayout but this one
 *  let you define specific methods for subclassed lights.
 */
class Light {
public:

    /* All lights have a name*/
    Light(const std::string& _name, bool _dynamic = false);

    virtual ~Light();

    /*  This name is used to construct the uniform name to be pass to the shader */
    virtual void setInstanceName(const std::string &_name);

    /*  Set Ambient Color. Which is constant across the scene */
    virtual void setAmbientColor(const glm::vec4 _ambient);

    /*  Set Diffuse Color. What we generaly understand for color of a light */
    virtual void setDiffuseColor(const glm::vec4 _diffuse);

    /*  Set Specular Color. This are the intense reflections of a light. AKA shinny spot */
    virtual void setSpecularColor(const glm::vec4 _specular);

    /*  Set the origin relative to which this light will be positioned */
    void setOrigin(LightOrigin origin);

    /*  Get the origin relative to which this light is positioned */
    LightOrigin getOrigin() const { return m_origin; }

    /*  Get the instances light name defined on the shader */
    virtual std::string getInstanceName();

    /*  Get the type of light, especially to identify the class and specific methods to it. */
    virtual LightType getType();

    /*  GLSL line to compute the specific light instance */
    virtual std::string getInstanceComputeBlock();

    /*
     * Inject the needed lines of GLSL code on the shader to make this light work
     * Returns LightUniforms for passing to setupProgram if this light is dynamic
     */
    virtual std::unique_ptr<LightUniforms> getUniforms() = 0;

    /*  Pass the uniforms for this particular DYNAMICAL light on the passed shader */
    virtual void setupProgram(RenderState& rs, const View& _view, ShaderProgram& _shader,
                              LightUniforms& _uniforms);

    /*  STATIC Function that compose sourceBlocks with Lights on a ProgramShader */
    static std::map<std::string, std::string>  assembleLights(const std::vector<std::unique_ptr<Light>>& _lights);

    bool isDynamic() const { return m_dynamic; }
protected:

    /*  Get the uniform name of the DYNAMICAL light */
    virtual std::string getUniformName();

    /*  Get the struct and function to compute a light */
    virtual std::string getClassBlock() = 0;

    /*  Get the instances GLSL block where the light is defined inside the shader */
    virtual std::string getInstanceBlock();

    /*  Get the instances GLSL block where NON DYNAMICAL light values are assigned inside the shader */
    virtual std::string getInstanceAssignBlock();

    /*  GLSL #defines flags for the instance of this light */
    virtual std::string getInstanceDefinesBlock() = 0;

    /* Get the string name of the type of this light (as it would be declared in GLSL) */
    virtual const std::string& getTypeName() = 0;

    /*  The name reference to the uniform on the shader.  */
    std::string m_name;

    /* Light Colors */
    glm::vec4 m_ambient;
    glm::vec4 m_diffuse;
    glm::vec4 m_specular;

    /*  This is use to identify the type of light after been pull inside a vector of uniq_ptr of this abstract class*/
    LightType m_type;

    /*  This determines if postion and direction of the light is related to the camera, ground or world */
    LightOrigin m_origin;

    bool m_dynamic;

};

}
