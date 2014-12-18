#pragma once

#include "pointLight.h"

class SpotLight : public PointLight {
public:
    
    SpotLight(const std::string& _name, bool _dynamic = false);
    virtual ~SpotLight();

    /*  Set the direction of the light */
    virtual void setDirection(const glm::vec3& _dir);

    /*  Set the properties of the cutoff light cone */
    virtual void setCutOff(float _lightConeRadiants, float _lieghtExponent);
    
    /*  GLSL #defines with the NUMBER of lights of this type */
    static std::string getArrayDefinesBlock(int _numberOfLights);

    /*  GLSL #defines with the NUMBER of lights of this type */
    static std::string getArrayUniformBlock();

    /*  GLSL block code with structs and need functions for this light type */
    static std::string getClassBlock();

    virtual std::string getInstanceDefinesBlock() override;
    virtual std::string getInstanceAssignBlock() override;

    virtual void setupProgram( std::shared_ptr<ShaderProgram> _program ) override;
    
protected:
   	glm::vec3 m_direction;

   	float m_spotExponent;
    float m_spotCutoff;
    float m_spotCosCutoff;
};
