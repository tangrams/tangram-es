#pragma once

#include <functional>

namespace Tangram {

class RenderState;

class Disposer {

public:

    Disposer(RenderState& rs, std::function<void(RenderState&)> task);

    void dispatchToRenderThread();

private:
    RenderState& m_rs;
    std::function<void()> m_task;

};

}