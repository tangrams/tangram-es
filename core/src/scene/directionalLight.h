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

    virtual void setupProgram(const View& _view, ShaderProgram& _program) override;

protected:

    /*  GLSL block code with structs and need functions for this light type */
    virtual std::string getClassBlock() override;
    virtual std::string getInstanceDefinesBlock() override;
    virtual std::string getInstanceAssignBlock() override;
    virtual const std::string& getTypeName() override;

    static std::string s_classBlock;

    glm::vec3 m_direction;
    const std::string m_uDirection;

private:

    static std::string s_typeName;

};

}
