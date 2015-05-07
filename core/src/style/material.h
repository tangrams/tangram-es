/* MATERIAL 
-------------------------------
This openGL Material implementation follows from the WebGL version of Tangram 
( https://github.com/tangrams/tangram/wiki/Materials-Overview )
*/

#pragma once
 
#include <memory>
#include "glm/glm.hpp"
#include "util/shaderProgram.h"
#include "util/texture.h"

enum class MappingType {
    UV,
    PLANAR,
    TRIPLANAR,
    SPHEREMAP
};

class Material {
public:
    
    Material();
    
    virtual ~Material(){};

    /*  Emission color is by default disblable and vec4(0.0).
    *   By setting the property you will enable and require to reloading the shader. */
    void setEmission(const glm::vec4 _emission);
    void setEmission(const std::string &_file, MappingType _type = MappingType::PLANAR, glm::vec3 _scale = glm::vec3(1.), glm::vec4 _amount = glm::vec4(1.));
    void setEmission(std::shared_ptr<Texture> _texture, MappingType _type = MappingType::PLANAR, glm::vec3 _scale = glm::vec3(1.), glm::vec4 _amount = glm::vec4(1.));

    /*  Ambient color is by default disblable and vec4(1.0).
    *   By setting the property you will enable and require to reloading the shader. */
    void setAmbient(const glm::vec4 _ambient);
    void setAmbient(const std::string &_file, MappingType _type = MappingType::PLANAR, glm::vec3 _scale = glm::vec3(1.), glm::vec4 _amount = glm::vec4(1.));
    void setAmbient(std::shared_ptr<Texture> _texture, MappingType _type = MappingType::PLANAR, glm::vec3 _scale = glm::vec3(1.), glm::vec4 _amount = glm::vec4(1.));

    /*  Difuse color is by default enable and vec4(1.0).
    *   If you want to disable it you have to disableDiffuse and reloading the shader. */
    void setDiffuse(const glm::vec4 _diffuse);
    void setDiffuse(const std::string &_file, MappingType _type = MappingType::PLANAR, glm::vec3 _scale = glm::vec3(1.), glm::vec4 _amount = glm::vec4(1.));
    void setDiffuse(std::shared_ptr<Texture> _texture, MappingType _type = MappingType::PLANAR, glm::vec3 _scale = glm::vec3(1.), glm::vec4 _amount = glm::vec4(1.));

    /*  Specular color is by default disblable and vec4(0.2) with a shinny factor of 0.2.
    *   By setting the property you will enable and require to reloading the shader. */
    void setSpecular(const glm::vec4 _specular, float _shinnyFactor);
    void setSpecular(const std::string &_file, MappingType _type = MappingType::PLANAR, glm::vec3 _scale = glm::vec3(1.), glm::vec4 _amount = glm::vec4(1.));
    void setSpecular(std::shared_ptr<Texture> _texture, MappingType _type = MappingType::PLANAR, glm::vec3 _scale = glm::vec3(1.), glm::vec4 _amount = glm::vec4(1.));

    /* Enable or disable emission colors */
    void setEmissionEnabled(bool _enable);

    /* Enable or disable ambient colors */
    void setAmbientEnabled(bool _enable);

    /* Enable or disable diffuse colors */
    void setDiffuseEnabled(bool _enable);

    /* Enable or disable specular colors */
    void setSpecularEnabled(bool _enable);

    void setNormal(const std::string &_file, MappingType _type = MappingType::PLANAR, glm::vec3 _scale = glm::vec3(1.), float _amount = 1.);
    void setNormal(std::shared_ptr<Texture> _texture, MappingType _type = MappingType::PLANAR, glm::vec3 _scale = glm::vec3(1.), float _amount = 1.);

    /*  Inject the needed lines of GLSL code on the shader to make this material work */
    virtual void injectOnProgram( std::shared_ptr<ShaderProgram> _shader );

    virtual void removeFromProgram( std::shared_ptr<ShaderProgram> _shader );

    /*  Method to pass it self as a uniform to the shader program */
    virtual void setupProgram( std::shared_ptr<ShaderProgram> _shader );
    
private:

    /* Get defines that need to be injected on top of the shader */
    virtual std::string getDefinesBlock();

    /* Get the GLSL struct and classes need to be injected */
    virtual std::string getClassBlock();

    std::string m_name;
    
    glm::vec4   m_emission;
    std::shared_ptr<Texture>   m_emission_texture;
    glm::vec3   m_emission_texture_scale;
    MappingType m_emission_texture_mapping;

    glm::vec4   m_ambient;
    std::shared_ptr<Texture>   m_ambient_texture;
    glm::vec3   m_ambient_texture_scale;
    MappingType m_ambient_texture_mapping;

    glm::vec4   m_diffuse;
    std::shared_ptr<Texture>   m_diffuse_texture;
    glm::vec3   m_diffuse_texture_scale;
    MappingType m_diffuse_texture_mapping;

    glm::vec4   m_specular;
    std::shared_ptr<Texture>   m_specular_texture;
    glm::vec3   m_specular_texture_scale;
    MappingType m_specular_texture_mapping;

    float       m_shininess;
    
    std::shared_ptr<Texture>   m_normal_texture;
    glm::vec3   m_normal_texture_scale;
    MappingType m_normal_texture_mapping;
    float       m_normal_texture_amount;

    bool        m_bEmission;
    bool        m_bAmbient;
    bool        m_bDiffuse;
    bool        m_bSpecular;
};
