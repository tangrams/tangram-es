#include "tangram.h"
#include "gl.h"

#include "text/fontContext.h"

#include "benchmark/benchmark_api.h"
#include "benchmark/benchmark.h"

using namespace Tangram;

static void BM_Tangram_LoadFont(benchmark::State& state) {
    while(state.KeepRunning()) {
        state.PauseTiming();
        std::shared_ptr<FontContext> m_ftContext;
        m_ftContext = FontContext::GetInstance();
        state.ResumeTiming();
        m_ftContext->addFont("FiraSans", "Medium", "");
    }
}
BENCHMARK(BM_Tangram_LoadFont);

BENCHMARK_MAIN();
