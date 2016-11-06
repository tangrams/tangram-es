#pragma once

#include "light.h"

namespace Tangram {

class AmbientLight : public Light {
public:

    AmbientLight(const std::string& _name, bool _dynamic = false);
    virtual ~AmbientLight();
    
    virtual void setupProgram(RenderState& rs, const View& _view, LightUniforms& _uniforms) override;

    std::unique_ptr<LightUniforms> getUniforms(ShaderProgram& _shader) override;

protected:

    /*  GLSL block code with structs and need functions for this light type */
    virtual void buildClassBlock(Material& _material, ShaderSource& _out) override;
    virtual const std::string& getTypeName() override;

private:

    static std::string s_typeName;
    
};

}
