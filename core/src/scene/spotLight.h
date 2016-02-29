#pragma once

#include "pointLight.h"
#include "glm/vec3.hpp"

namespace Tangram {

class SpotLight : public PointLight {
public:
    
    SpotLight(const std::string& _name, bool _dynamic = false);
    virtual ~SpotLight();

    /*  Set the direction of the light */
    virtual void setDirection(const glm::vec3& _dir);

    /*  Set the properties of the cutoff light cone */
    virtual void setCutoffAngle(float _cutoffConeDegrees);
    
    virtual void setCutoffExponent(float _exponent);
    
    virtual void setupProgram(const View& _view, ShaderProgram& _program) override;
    
protected:
    /*  GLSL block code with structs and need functions for this light type */
    virtual std::string getClassBlock() override;
    virtual std::string getInstanceAssignBlock() override;
    virtual const std::string& getTypeName() override;
    
    static std::string s_classBlock;
    
   	glm::vec3 m_direction;

   	float m_spotExponent;
    float m_spotCutoff;
    float m_spotCosCutoff;

private:

    static std::string s_typeName;

    UniformEntries::EntryId m_directionUniform = 0;
    UniformEntries::EntryId m_spotCosCutoffUniform = 0;
    UniformEntries::EntryId m_spotExponentUniform = 0;
    
};

}
