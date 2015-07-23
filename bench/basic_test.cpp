#include "tangram.h"
#include "gl.h"

#include "text/fontContext.h"

#include "benchmark/benchmark_api.h"

using namespace Tangram;

static void BM_Tangram_LoadFont(benchmark::State& state) {
    while(state.KeepRunning()) {
        std::shared_ptr<FontContext> m_ftContext;

        m_ftContext = std::make_shared<FontContext>();
        m_ftContext->addFont("FiraSans-Medium.ttf", "FiraSans");
    }
}

BENCHMARK(BM_Tangram_LoadFont);

BENCHMARK_MAIN();
