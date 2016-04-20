#include "catch.hpp"
#include "scene/styleMixer.h"
#include "yaml-cpp/yaml.h"
#include <iostream>
#include <set>

using namespace Tangram;
using YAML::Node;

TEST_CASE("Find the correct set of styles to mix", "[mixing][yaml]") {

    StyleMixer mixer;

    Node stylesMap = YAML::Load(R"END(
        styleA:
            base: baseA
        styleB:
            mix: styleA
        styleC:
            base: baseC
            mix: [styleB, styleA]
        )END");

    std::vector<std::string> correctMixesA = { "baseA" };
    std::vector<std::string> correctMixesB = { "styleA" };
    std::vector<std::string> correctMixesC = { "baseC", "styleB", "styleA" };

    auto resultMixesA = mixer.getStylesToMix(stylesMap["styleA"]);
    auto resultMixesB = mixer.getStylesToMix(stylesMap["styleB"]);
    auto resultMixesC = mixer.getStylesToMix(stylesMap["styleC"]);

    REQUIRE(resultMixesA == correctMixesA);
    REQUIRE(resultMixesB == correctMixesB);
    REQUIRE(resultMixesC == correctMixesC);

}

TEST_CASE("Find the correct order of styles to mix", "[mixing][yaml]") {

    StyleMixer mixer;

    Node stylesMap = YAML::Load(R"END(
        styleA:
            base: baseA
        styleB:
            mix: styleA
        styleC:
            base: styleA
            mix: [styleA, styleD]
        styleD:
            mix: [styleB, styleA]
        )END");

    std::vector<std::string> correctMixOrder = { "baseA", "styleA", "styleB", "styleD", "styleC" };

    auto resultMixOrder = mixer.getMixingOrder(stylesMap);

    REQUIRE(resultMixOrder == correctMixOrder);

}

TEST_CASE("Correctly mix two shader configuration nodes", "[mixing][yaml]") {

    StyleMixer mixer;

    Node shadersMap = YAML::Load(R"END(
        A:
            extensions: gl_arb_stuff
            uniforms:
                green: 0x00ff00
                tex: a.png
            blocks:
                color: colorBlockA
                normal: normalBlockA
        B:
            extensions: [amd_stuff, nv_stuff]
            uniforms:
                red: 0xff0000
                tex: b.png
            blocks:
                color: colorBlockB
                position: posBlockB
                global: globalBlockB
        C:
            blocks:
                color: colorBlockC
        D:
            blocks:
                color: colorBlockD
        E:
        )END");

    // Mixing is applied in-place, so independent tests need to take copies of the original nodes.

    Node resultAB;
    {
        Node shaders = Clone(shadersMap);
        mixer.applyShaderMixins(shaders["B"], {});
        mixer.applyShaderMixins(shaders["A"], { shaders["B"] });
        resultAB = shaders["A"];
        // std::cout << "### B mixed into A ###\n" << Dump(shaders) << std::endl;
    }
    Node resultBA;
    {
        Node shaders = Clone(shadersMap);
        mixer.applyShaderMixins(shaders["A"], {});
        mixer.applyShaderMixins(shaders["B"], { shaders["A"] });
        resultBA = shaders["B"];
        // std::cout << "### A mixed into B ###\n" << Dump(shaders) << std::endl;
    }

    REQUIRE(resultAB["uniforms"]["red"].Scalar() == "0xff0000");
    REQUIRE(resultBA["uniforms"]["red"].Scalar() == "0xff0000");

    REQUIRE(resultAB["uniforms"]["green"].Scalar() == "0x00ff00");
    REQUIRE(resultBA["uniforms"]["green"].Scalar() == "0x00ff00");

    REQUIRE(resultAB["uniforms"]["tex"].Scalar() == "a.png");
    REQUIRE(resultBA["uniforms"]["tex"].Scalar() == "b.png");

    REQUIRE(resultAB["blocks_mixed"]["color"].size() == 2);
    REQUIRE(resultAB["blocks_mixed"]["color"][0].Scalar() == "colorBlockB");
    REQUIRE(resultAB["blocks_mixed"]["color"][1].Scalar() == "colorBlockA");

    REQUIRE(resultBA["blocks_mixed"]["color"].size() == 2);
    REQUIRE(resultBA["blocks_mixed"]["color"][0].Scalar() == "colorBlockA");
    REQUIRE(resultBA["blocks_mixed"]["color"][1].Scalar() == "colorBlockB");

    REQUIRE(resultAB["blocks_mixed"]["normal"].size() == 1);
    REQUIRE(resultAB["blocks_mixed"]["normal"][0].Scalar() == "normalBlockA");

    REQUIRE(resultBA["blocks_mixed"]["normal"].size() == 1);
    REQUIRE(resultBA["blocks_mixed"]["normal"][0].Scalar() == "normalBlockA");

    REQUIRE(resultBA["blocks_mixed"]["global"].size() == 1);
    REQUIRE(resultBA["blocks_mixed"]["global"][0].Scalar() == "globalBlockB");

    // Mixed extensions are not in any particular order, so we check them against a set.
    std::set<std::string> correctExtensions = { "gl_arb_stuff", "amd_stuff", "nv_stuff" };

    std::set<std::string> resultExtensionsAB;
    for (const auto& ext : resultAB["extensions_mixed"]) {
        resultExtensionsAB.insert(ext.Scalar());
    }
    std::set<std::string> resultExtensionsBA;
    for (const auto& ext : resultBA["extensions_mixed"]) {
        resultExtensionsBA.insert(ext.Scalar());
    }

    REQUIRE(resultExtensionsAB == correctExtensions);
    REQUIRE(resultExtensionsBA == correctExtensions);

    Node resultABCD;
    {
        Node shaders = Clone(shadersMap);
        mixer.applyShaderMixins(shaders["D"], {});
        mixer.applyShaderMixins(shaders["C"], { shaders["D"] });
        mixer.applyShaderMixins(shaders["B"], { shaders["D"] });
        mixer.applyShaderMixins(shaders["A"], { shaders["B"], shaders["C"] });
        resultABCD = shaders["A"];
        // std::cout << "### D mixed into B and C, B and C mixed into A ###\n" << Dump(shaders) << std::endl;
    }

    // Check that "diamond inheritance" doesn't result in repeated blocks.
    REQUIRE(resultABCD["blocks_mixed"]["color"].size() == 4);

    Node resultEA;
    {
        Node shaders = Clone(shadersMap);
        mixer.applyShaderMixins(shaders["A"], {});
        mixer.applyShaderMixins(shaders["E"], { shaders["A"] });
        resultEA = shaders["E"];
    }

    REQUIRE(resultEA["extensions_mixed"].IsSequence() == true);
    correctExtensions = { "gl_arb_stuff" };
    std::set<std::string> resultExtensionsEA;
    for (const auto& ext : resultEA["extensions_mixed"]) {
        resultExtensionsEA.insert(ext.Scalar());
    }
    REQUIRE(resultExtensionsEA == correctExtensions);
    REQUIRE(resultEA["uniforms"]["green"].Scalar() == "0x00ff00");
    REQUIRE(resultEA["uniforms"]["tex"].Scalar() == "a.png");
    REQUIRE(resultEA["blocks_mixed"]["color"].size() == 1);
    REQUIRE(resultEA["blocks_mixed"]["color"][0].Scalar() == "colorBlockA");
    REQUIRE(resultEA["blocks_mixed"]["normal"].size() == 1);
    REQUIRE(resultEA["blocks_mixed"]["normal"][0].Scalar() == "normalBlockA");
}

TEST_CASE("Correctly mix two style config nodes", "[yaml][mixing]") {

    StyleMixer mixer;

    Node stylesMap = YAML::Load(R"END(
        styleA:
            base: baseA
            animated: false
            texcoords: false
            lighting: vertex
            texture: a.png
            # blend: none
            blend_order: 1
            material:
                diffuse: 0.5
                specular: mat.png
        styleB:
            base: baseB
            animated: true
            texcoords: false
            lighting: fragment
            texture: b.png
            blend: overlay
            blend_order: 2
            material:
                diffuse: mat.png
                specular: green
        )END");

    Node resultAB;
    {
        Node styles = Clone(stylesMap);
        mixer.applyStyleMixins(styles["styleA"], { styles["styleB"] });
        resultAB = styles["styleA"];
    }
    Node resultBA;
    {
        Node styles = Clone(stylesMap);
        mixer.applyStyleMixins(styles["styleB"], { styles["styleA"] });
        resultBA = styles["styleB"];
    }

    REQUIRE(resultAB["base"].Scalar() == "baseA");
    REQUIRE(resultBA["base"].Scalar() == "baseB");

    REQUIRE(resultAB["animated"].Scalar() == "true");
    REQUIRE(resultBA["animated"].Scalar() == "true");

    REQUIRE(resultAB["texcoords"].Scalar() == "false");
    REQUIRE(resultBA["texcoords"].Scalar() == "false");

    REQUIRE(resultAB["lighting"].Scalar() == "vertex");
    REQUIRE(resultBA["lighting"].Scalar() == "fragment");

    REQUIRE(resultAB["texture"].Scalar() == "a.png");
    REQUIRE(resultBA["texture"].Scalar() == "b.png");

    REQUIRE(resultAB["blend"].Scalar() == "overlay");
    REQUIRE(resultBA["blend"].Scalar() == "overlay");

    REQUIRE(resultAB["blend_order"].Scalar() == "1");
    REQUIRE(resultBA["blend_order"].Scalar() == "2");

    REQUIRE(resultAB["material"]["diffuse"].Scalar() == "0.5");
    REQUIRE(resultBA["material"]["diffuse"].Scalar() == "mat.png");

    REQUIRE(resultAB["material"]["specular"].Scalar() == "mat.png");
    REQUIRE(resultBA["material"]["specular"].Scalar() == "green");

}

TEST_CASE("fix:mergeMapFieldTakingLast: Correctly mix two style config nodes", "[yaml][mixing]") {

    StyleMixer mixer;

    Node styles = YAML::Load(R"END(
        styleA:
        styleB:
            base: styleA
            animated: true
            texcoords: false
            lighting: fragment
            texture: b.png
            blend: overlay
            blend_order: 2
            material:
                diffuse: mat.png
                specular: green
        )END");

    mixer.applyStyleMixins(styles["styleB"], { styles["styleA"] });
    auto resultB = styles["styleB"];

    REQUIRE(resultB["base"].Scalar() == "styleA");
    REQUIRE(resultB["animated"].Scalar() == "true");
    REQUIRE(resultB["texcoords"].Scalar() == "false");
    REQUIRE(resultB["lighting"].Scalar() == "fragment");
    REQUIRE(resultB["texture"].Scalar() == "b.png");
    REQUIRE(resultB["blend"].Scalar() == "overlay");
    REQUIRE(resultB["blend_order"].Scalar() == "2");
    REQUIRE(resultB["material"]["diffuse"].Scalar() == "mat.png");
    REQUIRE(resultB["material"]["specular"].Scalar() == "green");

}
