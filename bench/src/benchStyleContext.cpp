#include "benchmark/benchmark.h"

#include "data/tileData.h"
#include "js/JavaScriptContext.h"
#include "js/DuktapeContext.h"
#include "js/JSCoreContext.h"
#include "scene/styleContext.h"

#define RUN(FIXTURE, NAME)                                              \
    BENCHMARK_DEFINE_F(FIXTURE, NAME)(benchmark::State& st) { while (st.KeepRunning()) { run(); } } \
    BENCHMARK_REGISTER_F(FIXTURE, NAME); //->Iterations(10000);

using namespace Tangram;

template<class Context>
class JSGetPropertyFixture : public benchmark::Fixture {
public:
    Context ctx;
    Feature feature;
    void SetUp(const ::benchmark::State& state) override {
        JavaScriptScope<Context> jsScope(ctx);
        ctx.setGlobalValue("language", jsScope.newString("en"));
        feature.props.set("name:en", "Ozymandias");
        feature.props.set("title", "King of Kings");
        feature.props.set("number", 17);
        ctx.setCurrentFeature(&feature);
        ctx.setFunction(0, "function () { return (language && feature['name:' + language]) || title; }");
        ctx.setFunction(1, "function () { return feature.order || feature.number; }");
    }
    __attribute__ ((noinline)) void run() {
        StyleParam::Value value;
        benchmark::DoNotOptimize(value);
        JavaScriptScope<Context> jsScope(ctx);
        value = jsScope.getFunctionResult(0).toString();
        value = (float)jsScope.getFunctionResult(1).toDouble();
     }
};

using DuktapeGetPropertyFixture = JSGetPropertyFixture<DuktapeContext>;
RUN(DuktapeGetPropertyFixture, DuktapeGetPropertyBench)

#ifdef TANGRAM_USE_JSCORE
using JSCoreGetPropertyFixture = JSGetPropertyFixture<JSCoreContext>;
RUN(JSCoreGetPropertyFixture, JSCoreGetPropertyBench)
#endif

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
