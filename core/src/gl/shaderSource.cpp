#include "gl/shaderSource.h"

#include <set>
#include <sstream>

#include "selection_fs.h"

namespace Tangram {

void ShaderSource::setSourceStrings(const std::string& _fragSrc, const std::string& _vertSrc){
    m_fragmentShaderSource = std::string(_fragSrc);
    m_vertexShaderSource = std::string(_vertSrc);
}

void ShaderSource::addSourceBlock(const std::string& _tagName, const std::string& _glslSource, bool _allowDuplicate){

    if (!_allowDuplicate) {
        for (auto& source : m_sourceBlocks[_tagName]) {
            if (_glslSource == source) {
                return;
            }
        }
    }

    size_t start = 0;
    std::string sourceBlock = _glslSource;

    // Certain graphics drivers have issues with shaders having line continuation backslashes "\".
    // Example raster.glsl was having issues on s6 and note2 because of the "\"s in the glsl file.
    // This also makes sure if any "\"s are present in the shaders coming from style sheet will be
    // taken care of.

    // Replace blackslash+newline with spaces (simplification of regex "\\\\\\s*\\n")
    while ((start = sourceBlock.find("\\\n", start)) != std::string::npos) {
        sourceBlock.replace(start, 2, "  ");
        start += 2;
    }

    m_sourceBlocks[_tagName].push_back(sourceBlock);
}

std::string ShaderSource::applySourceBlocks(const std::string& _source, bool _fragShader, bool _selection) const {

    std::stringstream sourceOut;
    std::set<std::string> pragmas;

    sourceOut << "#define TANGRAM_EPSILON 0.00001\n";
    sourceOut << "#define TANGRAM_WORLD_POSITION_WRAP 100000.\n";

    if (_fragShader) {
        sourceOut << "#define TANGRAM_FRAGMENT_SHADER\n";
    } else {
        float depthDelta = 2.f / (1 << 16);
        sourceOut << "#define TANGRAM_DEPTH_DELTA " << std::to_string(depthDelta) << '\n';
        sourceOut << "#define TANGRAM_VERTEX_SHADER\n";
    }

    if (_selection) {
        sourceOut <<  "#define TANGRAM_FEATURE_SELECTION\n";
    }

    std::stringstream sourceIn(_source);
    std::string line;

    while (std::getline(sourceIn, line)) {
        if (line.empty()) {
            continue;
        }

        sourceOut << line << '\n';

        char pragmaName[128];
        // NB: The initial whitespace is to skip any number of whitespace chars
        if (sscanf(line.c_str(), " #pragma tangram:%127s", pragmaName) == 0) {
            continue;
        }

        auto block = m_sourceBlocks.find(pragmaName);
        if (block == m_sourceBlocks.end()) {
            continue;
        }

        bool unique;
        std::tie(std::ignore, unique) = pragmas.emplace(pragmaName);
        if (!unique) {
            continue;
        }

        // insert blocks
        for (auto& s : block->second) {
            sourceOut << s << '\n';
        }
    }

    // for (auto& block : m_sourceBlocks) {
    //     if (pragmas.find(block.first) == pragmas.end()) {
    //         logMsg("Warning: expected pragma '%s' in shader source\n",
    //                block.first.c_str());
    //     }
    // }

    return sourceOut.str();
}

void ShaderSource::addExtensionDeclaration(const std::string& _extension) {
    std::ostringstream oss;
    oss << "#if defined(GL_ES) == 0 || defined(GL_" << _extension << ")\n";
    oss << "    #extension GL_" << _extension << " : enable\n";
    oss << "    #define TANGRAM_EXTENSION_" << _extension << '\n';
    oss << "#endif\n";

    addSourceBlock("extensions", oss.str());
}

std::string ShaderSource::buildSelectionFragmentSource() const {
    return SHADER_SOURCE(selection_fs);
}

}
