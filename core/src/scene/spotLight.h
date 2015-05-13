#pragma once

#include "pointLight.h"

class SpotLight : public PointLight {
public:
    
    SpotLight(const std::string& _name, bool _dynamic = false);
    virtual ~SpotLight();

    /*  Set the direction of the light */
    virtual void setDirection(const glm::vec3& _dir);

    /*  Set the properties of the cutoff light cone */
    virtual void setCutoffAngle(float _cutoffConeDegrees);
    
    virtual void setCutoffExponent(float _exponent);
    
    virtual void setupProgram(const std::shared_ptr<View>& _view, std::shared_ptr<ShaderProgram> _program) override;
    
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
    
};
