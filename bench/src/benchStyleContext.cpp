#include "benchmark/benchmark.h"

#include "scene/styleContext.h"
#include "data/tileData.h"

#define RUN(FIXTURE, NAME)                                              \
    BENCHMARK_DEFINE_F(FIXTURE, NAME)(benchmark::State& st) { while (st.KeepRunning()) { run(); } } \
    BENCHMARK_REGISTER_F(FIXTURE, NAME); //->Iterations(10000);

using namespace Tangram;

template<size_t jscontext>
class JSGetPropertyFixture : public benchmark::Fixture {
public:
    std::unique_ptr<StyleContext> ctx;
    Feature feature;
    void SetUp(const ::benchmark::State& state) override {
        ctx.reset(new StyleContext(jscontext));
        feature.props.set("message", "Hello World!");
        ctx->setFeature(feature);
        ctx->setFunctions({R"(function () { return feature.message; })"});
    }
    __attribute__ ((noinline)) void run() {
        StyleParam::Value value;
        benchmark::DoNotOptimize(value);
        ctx->evalStyle(0, StyleParamKey::text_source, value);
     }
};

using DuktapeGetPropertyFixture = JSGetPropertyFixture<0>;
RUN(DuktapeGetPropertyFixture, DuktapeGetPropertyBench)

using JSCoreGetPropertyFixture = JSGetPropertyFixture<1>;
RUN(JSCoreGetPropertyFixture, JSCoreGetPropertyBench)

class DirectGetPropertyFixture : public benchmark::Fixture {
public:
    Feature feature;
    void SetUp(const ::benchmark::State& state) override {
        feature.props.set("message", "Hello World!");
    }
    void TearDown(const ::benchmark::State& state) override {}
};
BENCHMARK_DEFINE_F(DirectGetPropertyFixture, DirectGetPropertyBench)(benchmark::State& st) {
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
BENCHMARK_REGISTER_F(DirectGetPropertyFixture, DirectGetPropertyBench);

BENCHMARK_MAIN();
