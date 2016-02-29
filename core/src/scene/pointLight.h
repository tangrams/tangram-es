#pragma once

#include "light.h"

namespace Tangram {

class PointLight : public Light {
public:

	PointLight(const std::string& _name, bool _dynamic = false);
	virtual ~PointLight();

    /*  Set the position relative to the camera */
    virtual void setPosition(const glm::vec3& _pos);

    /*  Set the constant attenuation */
    virtual void setAttenuation(float _att);

    /*  Set the constant outer radius or inner/outer radius*/
    virtual void setRadius(float _outer);
    virtual void setRadius(float _inner, float _outer);

    virtual void setupProgram(const View& _view, ShaderProgram& _program) override;

protected:

    /*  GLSL block code with structs and need functions for this light type */
    virtual std::string getClassBlock() override;
    virtual std::string getInstanceDefinesBlock() override;
    virtual std::string getInstanceAssignBlock() override;
    virtual const std::string& getTypeName() override;

    static std::string s_classBlock;

    glm::vec4 m_position;

    float m_attenuation;
    float m_innerRadius;
    float m_outerRadius;

private:

    static std::string s_typeName;

    UniformEntries::EntryId m_positionUniform = 0;
    UniformEntries::EntryId m_attenuationUniform = 0;
    UniformEntries::EntryId m_innerRadiusUniform = 0;
    UniformEntries::EntryId m_outerRadiusUniform = 0;

};

}
