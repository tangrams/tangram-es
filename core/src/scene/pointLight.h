#pragma once

#include "light.h"

class PointLight : public Light {
public:

	PointLight();
	virtual ~PointLight();
    
    /*  Set the position relative to the camera */
    void setPosition(const glm::vec3 &_pos );

    /*  Set the constant attenuation. Basically dim */
    void setConstantAttenuation(float _constantAtt);

    /*  Set the linear attenuation based on the distance */
    void setLinearAttenuantion(float _linearAtt);

    /*  Set the quadratic attenuation based on the distance */
    void setQuadreaticAttenuation(float _quadraticAtt);

    /*  Set the the constant, linear and quadratic attenuation. */
    void setAttenuation(float _constant , float _linear = 0.0, float _quadratic = 0.0);

    /*  GLSL #defines with the NUMBER of lights of this type */
    static std::string getArrayDefinesBlock(int _numberOfLights);

    /*  GLSL #defines with the NUMBER of lights of this type */
    static std::string getArrayUniformBlock();

    /*  GLSL block code with structs and need functions for this light type */
    static std::string getClassBlock();

    virtual std::string getInstanceDefinesBlock();

    virtual void setupProgram( ShaderProgram &_program );
    
protected:
    glm::vec4	m_position;

    float m_constantAttenuation;
    float m_linearAttenuation;
    float m_quadraticAttenuation;
};
