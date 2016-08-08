#pragma once

#include <functional>

namespace Tangram {

class RenderState;

class Disposer {

public:

    Disposer() : m_rs(nullptr){}

    Disposer(RenderState& _rs) : m_rs(&_rs) {}

    void operator()(std::function<void(RenderState&)> _task);

private:
    RenderState* m_rs = nullptr;
};

}
