#pragma once

#include "light.h"
#include "glm/vec3.hpp"

namespace Tangram {


class DirectionalLight : public Light {
public:

    DirectionalLight(const std::string& _name, bool _dynamic = false);
    virtual ~DirectionalLight();

    /*	Set the direction of the light */
    virtual void setDirection(const glm::vec3& _dir);

    virtual void setupProgram(RenderState& rs, const View& _view, LightUniforms& _uniforms) override;

    struct Uniforms : public LightUniforms {

        Uniforms(ShaderProgram& _shader, const std::string& _name)
            : LightUniforms(_shader, _name),
              direction(_name+".direction") {}

        UniformLocation direction;
    };


    std::unique_ptr<LightUniforms> getUniforms(ShaderProgram& _shader) override;

protected:

    /*  GLSL block code with structs and need functions for this light type */
    virtual void buildClassBlock(Material& _material, ShaderSource& _out) override;
    virtual void buildInstanceAssignBlock(ShaderSource& _out) override;
    virtual const std::string& getTypeName() override;

    glm::vec3 m_direction;

private:

    static std::string s_typeName;

};

}
