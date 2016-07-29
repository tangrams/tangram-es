#include "disposer.h"
#include "gl/renderState.h"

namespace Tangram {

Disposer::Disposer(RenderState& rs, std::function<void(RenderState&)> task) : m_rs(rs) {
    m_task = std::bind(task, std::ref(rs));
}

void Disposer::dispatchToRenderThread() {
    m_rs.jobQueue.add(m_task);
}

} // namespace Tangram
