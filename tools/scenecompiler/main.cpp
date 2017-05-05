#include "glsl_optimizer.h"
#include "mockPlatform.h"
#include "scene/sceneLoader.h"
#include "scene/styleContext.h"
#include "scene/scene.h"
#include "scene/dataLayer.h"
#include "style/style.h"
#include "gl/shaderProgram.h"
#include "log.h"

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
        LOGE("Error: Shader optimzation failed %s / %d\n",
             glslopt_get_log(shader), type);
    }
    glslopt_shader_delete (shader);

    return ok;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        LOGE("Missing filename argument\n");
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

    LOG("\n ----------- Process: %s -------------", argv[1]);

    std::string fileName = argv[1];

    // std::ifstream resource(argv[1], std::ifstream::ate);
    // std::string sceneFile;
    // sceneFile.reserve(resource.tellg());
    // resource.seekg(std::ios::beg);
    // sceneFile.assign((std::istreambuf_iterator<char>(resource)),
    //                  std::istreambuf_iterator<char>());
    // resource.close();
    auto scenePath = std::string(argv[1]);
    auto yaml = MockPlatform::getBytesFromFile(argv[1]);

    YAML::Node sceneNode;

    try { sceneNode = YAML::Load(yaml.data(), yaml.size()); }
    catch (YAML::ParserException e) {
        LOGE("Parsing scene config '%s'", e.what());
        return -1;
    }


    auto platform = std::make_shared<MockPlatform>();;
    auto scene = std::make_shared<Scene>(platform, scenePath);

    SceneLoader::loadScene(platform, scene);

    LOG("got styles: %d", scene->styles().size());

    YAML::Node stylesNode = sceneNode["styles"];
    YAML::Node newStyles;

    for (auto& style : scene->styles()) {
        static const auto builtIn = {
            "polygons", "lines", "points", "text", "debug", "debugtext"
        };

        // if (std::find(builtIn.begin(), builtIn.end(),
        //               style->getName()) != builtIn.end()) {
        //     continue;
        // }

        LOG("compile shader: %s", style->getName().c_str());

        // auto& shader = style->getShaderProgram();
        std::string vertSrc;
        std::string fragSrc;

        // shader->getSource(vertSrc, fragSrc);

        if (!optimizeShader(ctx, vertSrc, true)) {
            LOG("vert:\n%s", vertSrc.c_str());
        }
        if (!optimizeShader(ctx, fragSrc, false)) {
            LOG("frag:\n%s", fragSrc.c_str());
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

    LOG("\n ----------- Testing: %s -------------", argv[1]);

    StyleContext styleContext;
    styleContext.initFunctions(*scene);
    Feature feature;
    feature.props.set("name", "check");
    styleContext.setFeature(feature);
    styleContext.setKeyword("$zoom", 15);

    for (auto& layer : scene->layers()) {
        testLayer(layer, styleContext, 0);
    }

    return 0;
}

void testFilter(const Filter& filter, StyleContext& ctx, int level) {
    if (filter.data.is<Filter::Function>()) {
        auto id = filter.data.get<Filter::Function>().id;
        LOG("%*s filter function: %d", level+2, "", id);
        ctx.evalFilter(id);
    }
    if (filter.data.is<Filter::OperatorAll>()) {
        for (const auto& filt : filter.data.get<Filter::OperatorAll>().operands) {
            testFilter(filt, ctx, level + 2);
        }
    }
    if (filter.data.is<Filter::OperatorAny>()) {
        for (const auto& filt : filter.data.get<Filter::OperatorAny>().operands) {
            testFilter(filt, ctx, level + 2);
        }
    }
    if (filter.data.is<Filter::OperatorNone>()) {
        for (const auto& filt : filter.data.get<Filter::OperatorNone>().operands) {
            testFilter(filt, ctx, level + 2);
        }
    }
}

void testLayer(const SceneLayer& layer, StyleContext& ctx, int level) {

    LOG("%*s <layer: %s>", level, "", layer.name().c_str());

    testFilter(layer.filter(), ctx, level);

    for (auto& rule : layer.rules()) {
        for (auto& styleParam : rule.parameters) {
            if (styleParam.function >= 0) {

                LOG("%*s function: %s - %s", level+4, "", rule.name.c_str(),
                       StyleParam::keyName(styleParam.key).c_str());

                StyleParam::Value val;
                //ctx.evalStyle(styleParam.function, styleParam.key, val);
            }
        }
    }
    for (auto& sublayer : layer.sublayers()) {
        testLayer(sublayer, ctx, level+4);
    }
}
