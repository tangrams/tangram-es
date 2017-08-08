#pragma once

#include "map.h"

#include <cmath>
#include <functional>

namespace Tangram {

template<typename T>
T ease(T _start, T _end, float _t, EaseType _e) {
    float f = _t;
    switch (_e) {
        case EaseType::cubic: f = (-2 * f + 3) * f * f; break;
        case EaseType::quint: f = (6 * f * f - 15 * f + 10) * f * f * f; break;
        case EaseType::sine: f = 0.5 - 0.5 * cos(M_PI * f); break;
        default: break;
    }
    return _start + (_end - _start) * f;
}

struct Ease {

    float t;
    float d;
    EaseCb cb;
    EaseCancelCb cancelCb;
    EaseFinishCb finishCb;

    Ease() : t(0), d(0), cb([](float) {}), finishCb(nullptr) {}
    Ease(float _duration, EaseCb _cb) : t(-1), d(_duration), cb(_cb), finishCb(nullptr) {}
    Ease(float _duration, EaseCb _cb, EaseCancelCb _cancelCb, EaseFinishCb _finishCb) : t(-1), d(_duration), cb(_cb), cancelCb(_cancelCb), finishCb(_finishCb) {}

    bool finished() const { return t >= d; }

    void update(float _dt) {
        t = t < 0 ? 0 : std::fmin(t + _dt, d);
        cb(std::fmin(1, t / d));
    }

};

}
