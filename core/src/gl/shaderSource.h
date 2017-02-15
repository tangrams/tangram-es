#pragma once

#include <map>
#include <string>
#include <vector>

#define SHADER_SOURCE(NAME) ShaderSource::shaderSourceBlock(NAME ## _data, NAME ## _size)

namespace Tangram {

class ShaderSource {
public:

    // Set the vertex and fragment shader GLSL source to the given strings/
    void setSourceStrings(const std::string& _vertSrc, const std::string& _fragSrc);

    // Add a block of GLSL to be injected at "#pragma tangram: [_tagName]" in the shader sources.
    void addSourceBlock(const std::string& _tagName, const std::string& _glslSource,
                        bool _allowDuplicate = true);

    void addExtensionDeclaration(const std::string& _extension);

    const auto& getSourceBlocks() const { return m_sourceBlocks; }

    // Build vertex shader source
    std::string buildVertexSource() const {
        return applySourceBlocks(m_vertexShaderSource, false);
    }

    // Build fragment shader source
    std::string buildFragmentSource() const {
        return applySourceBlocks(m_fragmentShaderSource, true);
    }

    // Build selection vertex shader source
    std::string buildSelectionVertexSource() const  {
        return applySourceBlocks(m_vertexShaderSource, false, true);
    }

    // Build selection fragment shader source
    std::string buildSelectionFragmentSource() const;


    static std::string shaderSourceBlock(const unsigned char* data, size_t size) {
        std::string block;
        if (data[size - 1] == '\n') {
            block.append(reinterpret_cast<const char*>(data), size);
        } else {
            block.reserve(size + 2);
            block.append(reinterpret_cast<const char*>(data), size);
            block += '\n';
        }
        return block;
    }

private:

    std::string applySourceBlocks(const std::string& _source, bool _fragShader,
                                  bool _selection = false) const;

    std::map<std::string, std::vector<std::string>> m_sourceBlocks;

    std::string m_vertexShaderSource = "";
    std::string m_fragmentShaderSource = "";
};

}
