/* MATERIAL 
-------------------------------
This openGL Material implementation follows from the WebGL version of Tangram 
( https://github.com/tangrams/tangram/wiki/Materials-Overview )
*/

#pragma once
 
#include <memory>
#include <string>
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

class Texture;
class ShaderProgram;

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

    /*  Emission color is by default disabled and vec4(0.0).
     *  Setting this property enables it and changes require reloading the shader. */
    void setEmission(const glm::vec4 _emission);
    void setEmission(const std::string &_file, MappingType _type = MappingType::UV, glm::vec3 _scale = glm::vec3(1.), glm::vec4 _amount = glm::vec4(1.));
    void setEmission(std::shared_ptr<Texture> _texture, MappingType _type = MappingType::UV, glm::vec3 _scale = glm::vec3(1.), glm::vec4 _amount = glm::vec4(1.));

    /*  Ambient color is by default disabled and vec4(1.0).
     *  Setting this property enables it and changes require reloading the shader. */
    void setAmbient(const glm::vec4 _ambient);
    void setAmbient(const std::string &_file, MappingType _type = MappingType::UV, glm::vec3 _scale = glm::vec3(1.), glm::vec4 _amount = glm::vec4(1.));
    void setAmbient(std::shared_ptr<Texture> _texture, MappingType _type = MappingType::UV, glm::vec3 _scale = glm::vec3(1.), glm::vec4 _amount = glm::vec4(1.));

    /*  Diffuse color is by default enabled and vec4(1.0).
     *  Changes require reloading the shader. */
    void setDiffuse(const glm::vec4 _diffuse);
    void setDiffuse(const std::string &_file, MappingType _type = MappingType::UV, glm::vec3 _scale = glm::vec3(1.), glm::vec4 _amount = glm::vec4(1.));
    void setDiffuse(std::shared_ptr<Texture> _texture, MappingType _type = MappingType::UV, glm::vec3 _scale = glm::vec3(1.), glm::vec4 _amount = glm::vec4(1.));

    /*  Specular color is by default disabled and vec4(0.2) with a shininess factor of 0.2.
     *  Setting this property enables it and changes require reloading the shader. */
    void setSpecular(const glm::vec4 _specular, float _shininess);
    void setSpecular(const std::string &_file, MappingType _type = MappingType::UV, glm::vec3 _scale = glm::vec3(1.), glm::vec4 _amount = glm::vec4(1.));
    void setSpecular(std::shared_ptr<Texture> _texture, MappingType _type = MappingType::UV, glm::vec3 _scale = glm::vec3(1.), glm::vec4 _amount = glm::vec4(1.));

    /* Enable or disable emission colors */
    void setEmissionEnabled(bool _enable);

    /* Enable or disable ambient colors */
    void setAmbientEnabled(bool _enable);

    /* Enable or disable diffuse colors */
    void setDiffuseEnabled(bool _enable);

    /* Enable or disable specular colors */
    void setSpecularEnabled(bool _enable);

    void setNormal(const std::string &_file, MappingType _type = MappingType::UV, glm::vec3 _scale = glm::vec3(1.), float _amount = 1.);
    void setNormal(std::shared_ptr<Texture> _texture, MappingType _type = MappingType::UV, glm::vec3 _scale = glm::vec3(1.), float _amount = 1.);

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

    std::string m_name = "material";
    
    glm::vec4   m_emission;
    glm::vec3   m_emission_texture_scale = glm::vec3(1.f);
    MappingType m_emission_texture_mapping = MappingType::UV;
    std::shared_ptr<Texture> m_emission_texture;

    glm::vec4   m_ambient = glm::vec4(1.f);
    glm::vec3   m_ambient_texture_scale = glm::vec3(1.f);
    MappingType m_ambient_texture_mapping = MappingType::UV;
    std::shared_ptr<Texture> m_ambient_texture;

    glm::vec4   m_diffuse = glm::vec4(1.f);
    glm::vec3   m_diffuse_texture_scale = glm::vec3(1.f);
    MappingType m_diffuse_texture_mapping = MappingType::UV;
    std::shared_ptr<Texture> m_diffuse_texture;

    glm::vec4   m_specular = glm::vec4(.2f);
    glm::vec3   m_specular_texture_scale = glm::vec3(1.f);
    MappingType m_specular_texture_mapping = MappingType::UV;
    std::shared_ptr<Texture> m_specular_texture;

    glm::vec3   m_normal_texture_scale = glm::vec3(1.f);
    float       m_normal_texture_amount = 1.f;
    MappingType m_normal_texture_mapping = MappingType::UV;
    std::shared_ptr<Texture> m_normal_texture;

    float       m_shininess = .2f;
    bool        m_bEmission = false;
    bool        m_bAmbient = false;
    bool        m_bDiffuse = true;
    bool        m_bSpecular = false;
};
