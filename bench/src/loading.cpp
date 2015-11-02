#include "tangram.h"
#include "gl.h"

#include "text/fontContext.h"

#include "benchmark/benchmark_api.h"
#include "benchmark/benchmark.h"

using namespace Tangram;

static void BM_Tangram_LoadFont(benchmark::State& state) {
    auto m_ftContext = std::make_shared<FontContext>();

    while(state.KeepRunning()) {
        state.PauseTiming();
        state.ResumeTiming();
        m_ftContext->addFont("FiraSans", "Medium", "");
    }
}
BENCHMARK(BM_Tangram_LoadFont);

BENCHMARK_MAIN();
