#pragma once

#include "tangram.h"

#include <cmath>
#include <functional>

namespace Tangram {

using EaseCb = std::function<void (float)>;

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

    Ease() : t(0), d(0), cb([](float) {}) {}
    Ease(float _duration, EaseCb _cb) : t(-1), d(_duration), cb(_cb) {}

    bool finished() const { return t >= d; }

    void update(float _dt) {
        t = t < 0 ? 0 : std::fmin(t + _dt, d);
        cb(std::fmin(1, t / d));
    }

};

}
