#pragma once

#include "light.h"
#include "scene/styleParam.h"

namespace Tangram {

class PointLight : public Light {
public:

    PointLight(const std::string& _name, bool _dynamic = false);
    virtual ~PointLight();

    /*  Set the position relative to the camera */
    virtual void setPosition(UnitVec<glm::vec3> position);

    /*  Set the constant attenuation */
    virtual void setAttenuation(float _att);

    /*  Set the constant outer radius or inner/outer radius*/
    virtual void setRadius(float _outer);
    virtual void setRadius(float _inner, float _outer);

    virtual void setupProgram(RenderState& rs, const View& _view, LightUniforms& _uniforms) override;

    struct Uniforms : public LightUniforms {
        Uniforms(ShaderProgram& _shader, const std::string& _name)
            : LightUniforms(_shader, _name),
              position(_name+".position"),
              attenuation(_name+".attenuation"),
              innerRadius(_name+".innerRadius"),
              outerRadius(_name+".outerRadius") {}

        UniformLocation position;
        UniformLocation attenuation;
        UniformLocation innerRadius;
        UniformLocation outerRadius;
    };

    auto getPosition() const -> UnitVec<glm::vec3> { return m_position; }

    std::unique_ptr<LightUniforms> getUniforms(ShaderProgram& _shader) override;

protected:

    /*  GLSL block code with structs and need functions for this light type */
    virtual void buildClassBlock(Material& _material, ShaderSource& _out) override;
    virtual void buildInstanceAssignBlock(ShaderSource& _out) override;
    virtual const std::string& getTypeName() override;

    UnitVec<glm::vec3> m_position;

    float m_attenuation;
    float m_innerRadius;
    float m_outerRadius;

private:

    static std::string s_typeName;

};

}
