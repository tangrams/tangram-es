/* MATERIAL
-------------------------------
This openGL Material implementation follows from the WebGL version of Tangram
( https://github.com/tangrams/tangram/wiki/Materials-Overview )
*/

#pragma once

#include "gl/uniform.h"

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include <memory>
#include <string>

namespace Tangram {

class RenderState;
class Texture;
class ShaderProgram;
class ShaderSource;

enum class MappingType {
    uv,
    planar,
    triplanar,
    spheremap
};

struct MaterialTexture {
    std::shared_ptr<Texture> tex = nullptr;
    MappingType mapping = MappingType::uv;
    glm::vec3 scale = glm::vec3(1.f);
    glm::vec3 amount = glm::vec3(1.f);
};

struct MaterialUniforms {

    UniformLocation emission{"u_material.emission"};
    UniformLocation emissionTexture{"material_emission_texture"};
    UniformLocation emissionScale{"u_material.emissionScale"};

    UniformLocation ambient{"u_material.ambient"};
    UniformLocation ambientTexture{"u_material_ambient_texture"};
    UniformLocation ambientScale{"u_material.ambientScale"};

    UniformLocation diffuse{"u_material.diffuse"};
    UniformLocation diffuseTexture{"u_material_diffuse_texture"};
    UniformLocation diffuseScale{"u_material.diffuseScale"};

    UniformLocation specular{"u_material.specular"};
    UniformLocation shininess{"u_material.shininess"};

    UniformLocation specularTexture{"u_material_specular_texture"};
    UniformLocation specularScale{"u_material.specularScale"};

    UniformLocation normalTexture{"u_material_normal_texture"};
    UniformLocation normalScale{"u_material.normalScale"};
    UniformLocation normalAmount{"u_material.normalAmount"};
};

class Material {
public:

    Material();

    virtual ~Material(){};

    /*  Emission color is by default disabled and vec4(0.0).
     *  Setting this property enables it and changes require reloading the shader. */
    void setEmission(glm::vec4 _emission);
    void setEmission(MaterialTexture _emissionTexture);

    /*  Ambient color is by default disabled and vec4(1.0).
     *  Setting this property enables it and changes require reloading the shader. */
    void setAmbient(glm::vec4 _ambient);
    void setAmbient(MaterialTexture _ambientTexture);

    /*  Diffuse color is by default enabled and vec4(1.0).
     *  Changes require reloading the shader. */
    void setDiffuse(glm::vec4 _diffuse);
    void setDiffuse(MaterialTexture _diffuseTexture);

    /*  Specular color is by default disabled and vec4(0.2) with a shininess factor of 0.2.
     *  Setting this property enables it and changes require reloading the shader. */
    void setSpecular(glm::vec4 _specular);
    void setSpecular(MaterialTexture _specularTexture);

    void setShininess(float _shiny);

    /* Enable or disable emission colors */
    void setEmissionEnabled(bool _enable);

    /* Enable or disable ambient colors */
    void setAmbientEnabled(bool _enable);

    /* Enable or disable diffuse colors */
    void setDiffuseEnabled(bool _enable);

    /* Enable or disable specular colors */
    void setSpecularEnabled(bool _enable);

    void setNormal(MaterialTexture _normalTexture);

    /*  Inject the needed lines of GLSL code on the shader to make this material work */
    virtual std::unique_ptr<MaterialUniforms> injectOnProgram(ShaderSource& _shader);

    /*  Method to pass it self as a uniform to the shader program */
    virtual void setupProgram(RenderState& rs, ShaderProgram& _shader,
                              MaterialUniforms& _uniforms);

    bool hasEmission() const { return m_bEmission; }
    bool hasAmbient() const { return m_bAmbient; }
    bool hasDiffuse() const { return m_bDiffuse; }
    bool hasSpecular() const { return m_bSpecular; }

private:

    /* Get defines that need to be injected on top of the shader */
    std::string getDefinesBlock();

    /* Get the GLSL struct and classes need to be injected */
    std::string getClassBlock();

    bool m_bEmission = false;
    glm::vec4 m_emission = glm::vec4(1.f);
    MaterialTexture m_emission_texture;

    bool m_bAmbient = false;
    glm::vec4 m_ambient = glm::vec4(1.f);
    MaterialTexture m_ambient_texture;

    bool m_bDiffuse = true;
    glm::vec4 m_diffuse = glm::vec4(1.f);
    MaterialTexture m_diffuse_texture;

    bool m_bSpecular = false;
    glm::vec4 m_specular = glm::vec4(.2f);
    MaterialTexture m_specular_texture;

    MaterialTexture m_normal_texture;

    float m_shininess = .2f;
};

}
