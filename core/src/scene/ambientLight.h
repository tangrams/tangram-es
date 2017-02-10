#pragma once

#include "scene/light.h"

namespace Tangram {

class AmbientLight : public Light {
public:

    AmbientLight(const std::string& _name, bool _dynamic = false);
    virtual ~AmbientLight();

    virtual void setupProgram(RenderState& rs, const View& _view, ShaderProgram& _shader,
                              LightUniforms& _uniforms) override;

    std::unique_ptr<LightUniforms> getUniforms() override;

protected:

    /*  GLSL block code with structs and need functions for this light type */
    virtual std::string getClassBlock() override;
    virtual std::string getInstanceDefinesBlock() override;
    virtual std::string getInstanceAssignBlock() override;
    virtual const std::string& getTypeName() override;

private:

    static std::string s_typeName;

};

}
