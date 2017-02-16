#pragma once

#include "scene/pointLight.h"

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

    virtual void setupProgram(RenderState& rs, const View& _view, ShaderProgram& _shader,
                              LightUniforms& _uniforms) override;

    struct Uniforms : public PointLight::Uniforms {

        Uniforms(const std::string& _name)
            : PointLight::Uniforms(_name),
            direction(_name+".direction"),
            spotCosCutoff(_name+".spotCosCutoff"),
            spotExponent(_name+".spotExponent") {}

        UniformLocation direction;
        UniformLocation spotCosCutoff;
        UniformLocation spotExponent;
    };

    std::unique_ptr<LightUniforms> getUniforms() override;

protected:
    /*  GLSL block code with structs and need functions for this light type */
    virtual std::string getClassBlock() override;
    virtual std::string getInstanceAssignBlock() override;
    virtual const std::string& getTypeName() override;

    glm::vec3 m_direction;

    float m_spotExponent;
    float m_spotCutoff;
    float m_spotCosCutoff;

private:

    static std::string s_typeName;

};

}
