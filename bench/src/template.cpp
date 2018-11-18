#include "benchmark/benchmark.h"


using namespace Tangram;

class TemplateFixture : public benchmark::Fixture {
public:
    // std::unique_ptr<Template> ctx;

    void SetUp() override {
        ctx = std::make_unique<TestContext>();
    }
    void TearDown() override {
        ctx.reset();
    }
};

BENCHMARK_DEFINE_F(TemplateFixture, TemplateMethod)(benchmark::State& st) {
    while (st.KeepRunning()) {
        ctx.doSuff();
    }
}

BENCHMARK_REGISTER_F(TemplateFixture, TemplateMethod);

BENCHMARK_MAIN();
