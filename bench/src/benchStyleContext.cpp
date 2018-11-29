#include "benchmark/benchmark.h"

#include "scene/styleContext.h"
#include "data/tileData.h"

using namespace Tangram;

class JSGetPropertyBenchFixture : public benchmark::Fixture {
public:
    StyleContext ctx;
    Feature feature;
    void SetUp(const ::benchmark::State& state) override {
        feature.props.set("message", "Hello World!");

        ctx.setFeature(feature);
        ctx.setFunctions({R"(function () { return feature.message; })"});
    }
    void TearDown(const ::benchmark::State& state) override {}
};
BENCHMARK_DEFINE_F(JSGetPropertyBenchFixture, JSGetPropertyBench)(benchmark::State& st) {
    StyleParam::Value value;

    while (st.KeepRunning()) {
        ctx.evalStyle(0, StyleParamKey::text_source, value);
    }
}
BENCHMARK_REGISTER_F(JSGetPropertyBenchFixture, JSGetPropertyBench)->Iterations(10000);



class DirectGetPropertyBenchFixture : public benchmark::Fixture {
public:
    Feature feature;
    void SetUp(const ::benchmark::State& state) override {
        feature.props.set("message", "Hello World!");
    }
    void TearDown(const ::benchmark::State& state) override {}
};
BENCHMARK_DEFINE_F(DirectGetPropertyBenchFixture, DirectGetPropertyBench)(benchmark::State& st) {
    StyleParam::Value value;

    while (st.KeepRunning()) {
        const auto v = feature.props.get("message");
        if (v.is<std::string>()) {
            value = v.get<std::string>();
        } else {
            printf("eerrr\n");
        }
    }
}
BENCHMARK_REGISTER_F(DirectGetPropertyBenchFixture, DirectGetPropertyBench);

BENCHMARK_MAIN();
