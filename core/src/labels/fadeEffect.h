#pragma once

#include <cmath>
#ifndef M_PI // M_PI is non-standard since c++99
#define M_PI (3.14159265358979323846264338327950288)
#endif

namespace Tangram {

struct FadeEffect {

public:

    enum Interpolation {
        linear = 0,
        pow,
        sine
    };

    FadeEffect() {}

    FadeEffect(bool _in, Interpolation _interpolation, float _duration)
        : m_interpolation(_interpolation), m_duration(_duration), m_in(_in) {}

    float update(float _dt) {
        m_step += _dt;
        float st = m_step / m_duration;

        switch (m_interpolation) {
            case Interpolation::linear:
                return m_in ? st : -st + 1;
            case Interpolation::pow:
                return m_in ? st * st : -(st * st) + 1;
            case Interpolation::sine:
                return m_in ? sin(st * M_PI * 0.5) : cos(st * M_PI * 0.5);
        }

        return st;
    }

    void reset(bool _in, Interpolation _interpolation, float _duration) {
        m_in = _in;
        m_interpolation = _interpolation;
        m_duration = _duration;
        m_step = 0.f;
    }

    bool isFinished() {
        return m_step > m_duration;
    }

private:

    Interpolation m_interpolation = Interpolation::linear;
    float m_duration = 0.0f;
    float m_step = 0.0f;
    bool m_in = false;
};

}

