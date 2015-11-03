#pragma once

#include <cmath>
#include <functional>

namespace Tangram {

using EaseCb = std::function<void (float)>;

struct Ease {

    float t;
    float d;
    EaseCb cb;

    Ease(float _duration, EaseCb _cb) : t(0), d(_duration), cb(_cb) {}

    bool finished() { return t >= d; }

    void update(float _dt) {
        t = fminf(t + _dt, d);
        cb(t / d);
    }

};

template<typename T>
T ease(T _start, T _end, float t) {
    return _start * (1.f - t) + _end * t;
}

}