#include "gl.h"
#include "platform.h"
#include "tangram.h"
#include "gl/shaderProgram.h"

#include "benchmark/benchmark_api.h"
#include "benchmark/benchmark.h"

using namespace Tangram;

#define BENCH_UNIFORM_ENTRY

ShaderProgram program;
float value = 0.f;

#ifdef BENCH_UNIFORM_ENTRY
UniformEntries::EntryId entry = 0;
#endif

static void BM_Tangram_SetUniform(benchmark::State& state) {
#ifdef BENCH_UNIFORM_ENTRY
    UniformEntries::lazyGenEntry(&entry, "u_uniformValue");

    while(state.KeepRunning()) {
        value += 0.5;
        program.setUniformf(UniformEntries::getEntry(entry), value);
    }
#else
    while(state.KeepRunning()) {
        value += 0.5;
        program.setUniformf("u_uniformValue", value);
    }

#endif
}
BENCHMARK(BM_Tangram_SetUniform);

BENCHMARK_MAIN();
