#include "glsl_optimizer.h"
#include "platform.h"
#include "scene/sceneLoader.h"
#include "scene/styleContext.h"
#include "scene/scene.h"
#include "scene/dataLayer.h"
#include "style/style.h"
#include "gl/shaderProgram.h"

#include <fstream>
#include <algorithm>

using namespace Tangram;

void testLayer(const SceneLayer& layer, StyleContext& ctx, int level);

bool optimizeShader(glslopt_ctx* ctx, std::string& source, bool vertexShader) {

    const glslopt_shader_type type = vertexShader
        ? kGlslOptShaderVertex
        : kGlslOptShaderFragment;

    bool ok = false;

    glslopt_shader *shader = glslopt_optimize (ctx, type, source.c_str(), 0);
    if (glslopt_get_status (shader)) {
        source = glslopt_get_output(shader);
        ok = true;
    } else {
        logMsg("Error: Shader optimzation failed %s / %d\n",
               glslopt_get_log(shader), type);
    }
    glslopt_shader_delete (shader);

    return ok;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        logMsg("Missing filename argument\n");
        return 1;
    }
    glslopt_target languageTarget = kGlslTargetOpenGL;
    std::string prefix = "gl_";

    if (argc >= 3) {
        if (std::strcmp("es2", argv[2]) == 0) {
            languageTarget = kGlslTargetOpenGLES20;
            prefix = "es2_";
        } else if (std::strcmp("es3", argv[2]) == 0) {
            languageTarget = kGlslTargetOpenGLES30;
            prefix = "es3_";
        }
    }

    glslopt_ctx* ctx = glslopt_initialize(languageTarget);

    logMsg("\n ----------- Process: %s ------------- \n", argv[1]);

    std::string fileName = argv[1];

    // std::ifstream resource(argv[1], std::ifstream::ate);
    // std::string sceneFile;
    // sceneFile.reserve(resource.tellg());
    // resource.seekg(std::ios::beg);
    // sceneFile.assign((std::istreambuf_iterator<char>(resource)),
    //                  std::istreambuf_iterator<char>());
    // resource.close();
    auto sceneRelPath = setResourceRoot(argv[1]);
    auto sceneString = stringFromFile(sceneRelPath.c_str(), PathType::resource);

    Scene scene;
    YAML::Node sceneNode;

    try { sceneNode = YAML::Load(sceneString); }
    catch (YAML::ParserException e) {
        LOGE("Parsing scene config '%s'", e.what());
        return false;
    }

    SceneLoader::loadScene(sceneNode, scene);

    logMsg("got styles: %d\n", scene.styles().size());

    YAML::Node stylesNode = sceneNode["styles"];
    YAML::Node newStyles;

    for (auto& style : scene.styles()) {
        static const auto builtIn = {
            "polygons", "lines", "points", "text", "debug", "debugtext"
        };

        if (std::find(builtIn.begin(), builtIn.end(),
                      style->getName()) != builtIn.end()) {
            continue;
        }

        logMsg("compile shader: %s\n", style->getName().c_str());

        auto& shader = style->getShaderProgram();
        std::string vertSrc;
        std::string fragSrc;

        shader->getSource(vertSrc, fragSrc);

        if (!optimizeShader(ctx, vertSrc, true)) {
            logMsg("vert:\n%s\n", vertSrc.c_str());
        }
        if (!optimizeShader(ctx, fragSrc, false)) {
            logMsg("frag:\n%s\n", fragSrc.c_str());
        }

        YAML::Node styleNode = stylesNode[style->getName()];
        YAML::Node shaderNode = styleNode["shaders"];
        YAML::Node blocksNode;

        blocksNode["vertex_shader"] = vertSrc;
        blocksNode["fragment_shader"] = fragSrc;

        // overwrite old blocks
        shaderNode["blocks"]  = blocksNode;

        newStyles[style->getName()] = styleNode;
    }

    sceneNode["styles"] = newStyles;

    glslopt_cleanup(ctx);

    std::ofstream fout(prefix + fileName.substr(fileName.find_last_of("/")+1));
    fout << sceneNode;

    logMsg("\n ----------- Testing: %s ------------- \n", argv[1]);

    StyleContext styleContext;
    styleContext.initFunctions(scene);
    Feature feature;
    feature.props.add("name", "check");
    styleContext.setFeature(feature);
    styleContext.setGlobal("$zoom", 15);

    for (auto& layer : scene.layers()) {
        testLayer(layer, styleContext, 0);
    }

    return 0;
}

void testFilter(const Filter& filter, StyleContext& ctx, int level) {
    if (filter.type == FilterType::function) {
        auto id = filter.data.get<Filter::Function>().id;
        logMsg("%*s filter function: %d\n", level+2, "", id);
        ctx.evalFilter(id);
    }
    if (filter.type == FilterType::all ||
        filter.type == FilterType::none ||
        filter.type == FilterType::any) {
        for (const auto& filt : filter.data.get<Filter::Operator>().operands) {
            testFilter(filt, ctx, level + 2);
        }
    }
}

void testLayer(const SceneLayer& layer, StyleContext& ctx, int level) {

    logMsg("%*s <layer: %s>\n", level, "", layer.name().c_str());

    testFilter(layer.filter(), ctx, level);

    for (auto& rule : layer.rules()) {
        for (auto& styleParam : rule.parameters) {
            if (styleParam.function >= 0) {

                logMsg("%*s function: %s - %s\n", level+4, "", rule.name.c_str(),
                       StyleParam::getKeyName(styleParam.key).c_str());

                StyleParam::Value val;
                ctx.evalStyle(styleParam.function, styleParam.key, val);
            }
        }
    }
    for (auto& sublayer : layer.sublayers()) {
        testLayer(sublayer, ctx, level+4);
    }
}
