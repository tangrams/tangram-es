#pragma once

#include "light.h"

class PointLight : public Light {
public:

	PointLight(const std::string& _name, bool _dynamic = false);
	virtual ~PointLight();
    
    /*  Set the position relative to the camera */
    virtual void setPosition(const glm::vec3& _pos );

    /*  Set the constant attenuation. Basically dim */
    virtual void setConstantAttenuation(float _constantAtt);

    /*  Set the linear attenuation based on the distance */
    virtual void setLinearAttenuation(float _linearAtt);

    /*  Set the quadratic attenuation based on the distance */
    virtual void setQuadreaticAttenuation(float _quadraticAtt);

    /*  Set the the constant, linear and quadratic attenuation. */
    virtual void setAttenuation(float _constant , float _linear , float _quadratic );

    /*  GLSL block code with structs and need functions for this light type */
    static std::string getClassBlock();

    virtual std::string getInstanceDefinesBlock() override;
    virtual std::string getInstanceAssignBlock() override;
    
    virtual void setupProgram( ShaderProgram* _program ) override;
    
protected:
    glm::vec4	m_position;

    float m_constantAttenuation;
    float m_linearAttenuation;
    float m_quadraticAttenuation;
};
