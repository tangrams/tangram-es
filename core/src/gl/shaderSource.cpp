#include "gl/shaderSource.h"
#include "util/floatFormatter.h"

#include <set>
#include <sstream>
#include <tuple>

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

    std::string out;
    std::set<std::string> pragmas;

    out.append("#define TANGRAM_EPSILON 0.00001\n");
    out.append("#define TANGRAM_WORLD_POSITION_WRAP 100000.\n");

    if (_fragShader) {
        out.append("#define TANGRAM_FRAGMENT_SHADER\n");
    } else {
        out.append("#define TANGRAM_DEPTH_DELTA 0.00003052\n"); // 2^-15
        out.append("#define TANGRAM_VERTEX_SHADER\n");
    }
    if (_selection) {
        out.append("#define TANGRAM_FEATURE_SELECTION\n");
    }

    size_t shaderLength = out.length() + _source.length();
    for (auto& block : m_sourceBlocks) {
        for (auto& s : block.second) {
            shaderLength += s.length() + 1;
        }
        shaderLength += 1;
    }
    out.reserve(shaderLength);

    size_t end = 0;
    const char* str = _source.c_str();

    while (true) {
        size_t start = end;

        auto pragma = _source.find("#pragma ", start);
        if (pragma == std::string::npos) {
            // Write appendix and done
            out.append(str + start);
            break;
        }

        // find end of #pragma line
        end = _source.find('\n', pragma);
        if (end == std::string::npos) {
            end = _source.length();
        }

        // Write everything to end of #pragma line
        out.append(str + start, end - start);
        if (out.back() != '\n') { out.append("\n"); }

        char pragmaName[128];
        if (sscanf(str + pragma + 8, " tangram:%127s", pragmaName) == 0) {
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
            if (s.empty()) { continue; }

            out.append(s);

            if (out.back() != '\n') { out.append("\n"); }
        }
    }

    // printf("overalloc'd: %d\n",  shaderLength - int(out.length()));
    // for (auto& block : m_sourceBlocks) {
    //     if (pragmas.find(block.first) == pragmas.end()) {
    //         logMsg("Warning: expected pragma '%s' in shader source\n",
    //                block.first.c_str());
    //     }
    // }
    return out;
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
    return selection_fs;
}

}
