#include "tangram.h"
#include "gl.h"

#include "util/builders.h"
#include "glm/glm.hpp"
#include <vector>

#include "benchmark/benchmark_api.h"
#include "benchmark/benchmark.h"

using namespace Tangram;

std::vector<glm::vec3> line = {
    {0.0, 0.0, 0.0},
    {1.0, 0.0, 0.0},
    {1.0, 1.0, 0.0},
    {0.0, 1.0, 0.0},
};

struct PosNormEnormColVertex {
    glm::vec3 pos;
    glm::vec2 texcoord;
    glm::vec2 enorm;
    GLfloat ewidth;
    GLuint abgr;
    GLfloat layer;
};

static void BM_Tangram_BuildButtMiterLine(benchmark::State& state) {
    while(state.KeepRunning()) {
        std::vector<PosNormEnormColVertex> vertices;
        PolyLineBuilder builder {
            [&](const glm::vec3& coord, const glm::vec2& normal, const glm::vec2& uv) {
                vertices.push_back({ coord, uv, normal, 0.5f, 0xffffff, 0.f });
            },
            CapTypes::butt,
            JoinTypes::miter
        };

        Builders::buildPolyLine(line, builder);
    }
}
BENCHMARK(BM_Tangram_BuildButtMiterLine);

static void BM_Tangram_BuildRoundRoundLine(benchmark::State& state) {
    while(state.KeepRunning()) {
        std::vector<PosNormEnormColVertex> vertices;
        PolyLineBuilder builder {
            [&](const glm::vec3& coord, const glm::vec2& normal, const glm::vec2& uv) {
                vertices.push_back({ coord, uv, normal, 0.5f, 0xffffff, 0.f });
            },
            CapTypes::round,
            JoinTypes::round
        };

        Builders::buildPolyLine(line, builder);
    }
}
BENCHMARK(BM_Tangram_BuildRoundRoundLine);

BENCHMARK_MAIN();
