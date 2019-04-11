#include "benchmark/benchmark.h"

// Define iterations for fixed number runs()
#define NUM_ITERATIONS 0

#if (NUM_ITERATIONS > 0)
#define ITERATIONS ->Iterations(NUM_ITERATIONS)
#else
#define ITERATIONS
#endif

#define RUN(FIXTURE, NAME)                                              \
    BENCHMARK_DEFINE_F(FIXTURE, NAME)(benchmark::State& st) { while (st.KeepRunning()) { run(); } } \
    BENCHMARK_REGISTER_F(FIXTURE, NAME)ITERATIONS;


//using namespace Tangram;

class TemplateFixture : public benchmark::Fixture {
public:
    int count;
    void SetUp(const ::benchmark::State& state) override {
        count = 0;
    }

    void TearDown(const ::benchmark::State& state) override {
        count = 0; // :D
    }

    __attribute__ ((noinline))
    void run() {
        count++;
    }
};

RUN(TemplateFixture, TemplateBench)

BENCHMARK_MAIN();
