#include "gl/disposer.h"
#include "gl/renderState.h"

namespace Tangram {

void Disposer::operator()(std::function<void(RenderState&)> _task) {
    if (!m_rs) { return; }

    m_rs->jobQueue.add(std::bind(_task, std::ref(*m_rs)));
}

} // namespace Tangram
