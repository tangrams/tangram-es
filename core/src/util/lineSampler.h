#pragma once

#include "glm/glm.hpp"

namespace Tangram {

template<typename Points>
struct LineSampler {

    template<typename T>
    void set(std::vector<T> _points) {
        m_points.clear();

        if (_points.empty()) { return; }

        glm::vec2 p = { _points[0].x, _points[0].y };

        m_points.reserve(_points.size());
        m_points.emplace_back(p, 0.f);

        float sum = 0.f;

        for (size_t i = 0; i < _points.size()-1; i++) {
            glm::vec2 n = { _points[i+1].x, _points[i+1].y };
            float d = glm::distance(p, n);
            if (d > 0.f) {
                sum += d;
                m_points.push_back({n.x, n.y, sum});
            }
            p = n;
        }
        reset();
    }

    template<typename T>
    bool add(T _point) {

        glm::vec2 p = { _point.x, _point.y };

        if (m_points.empty()) {
            m_points.push_back(glm::vec3{p, 0.f});
            return true;
        }

        size_t i = m_points.size()-1;

        glm::vec2 prev = { m_points[i].x, m_points[i].y };
        float d = glm::distance(prev, p);

        if (d > 0.f) {
            m_points.push_back(glm::vec3{p, m_points[i].z + d});
            return true;
        }
        return false;
    }

    void clearPoints() {
        m_points.clear();
        reset();
    }

    void reset() {
        m_curAdvance = 0.f;
        m_curPoint = 0;
    }

    void reversePoints() {
        float sum = sumLength();

        std::reverse(m_points.begin(), m_points.end());
        for (auto& p : m_points) { p.z = sum - p.z; }

        m_curAdvance = 0.f;
        m_curPoint = 0;
    }

    float sumLength() {
        if (m_points.empty()) { return 0.f; }

        return m_points[m_points.size()-1].z;
    }

    size_t curSegment() {
        return m_curPoint;
    }

    glm::vec3 point(size_t _pos) {
        return m_points[_pos];
    }

    float segmentLength(size_t _pos) {
        if (_pos >= m_points.size()-1) { return 0; }

        return (m_points[_pos+1].z - m_points[_pos].z);
    }

    glm::vec2 segmentDirection(size_t _pos) {
        if (_pos >= m_points.size()-1) {
            _pos = m_points.size()-2;
        }

        return ((glm::vec2(m_points[_pos+1]) - glm::vec2(m_points[_pos])) /
                (m_points[_pos+1].z - m_points[_pos].z));
    }


    bool sample(float _offset, glm::vec2& _point, glm::vec2& _rotation) {
        return advance(_offset - m_curAdvance, _point, _rotation);
    }

    bool advance(float _amount, glm::vec2& _point, glm::vec2& _rotation) {

        if (m_curPoint >= m_points.size()-1) { return false; }

        float end = m_curAdvance + _amount;

        if (_amount > 0) {

            while (true) {
                const auto& curr = m_points[m_curPoint];
                const auto& next = m_points[m_curPoint+1];

                // needed length from cur point
                float length = end - curr.z;
                // length from cur to next point
                float segmentLength = next.z - curr.z;

                if (length <= segmentLength) {
                    float f = length / segmentLength;

                    _point = glm::vec2(curr) + (glm::vec2(next) - glm::vec2(curr)) * f;
                    _rotation = (glm::vec2(next) - glm::vec2(curr)) / segmentLength;

                    m_curAdvance = end;
                    return true;

                } else {
                    if (m_curPoint >= m_points.size()-2) {
                        _point = glm::vec2(next);
                        _rotation = (glm::vec2(next) - glm::vec2(curr)) / segmentLength;
                        m_curAdvance = sumLength();
                        return false;
                    }
                    m_curPoint += 1;
                }
            }
        } else {

            while (true) {
                const auto& curr = m_points[m_curPoint];
                const auto& next = m_points[m_curPoint+1];

                // needed length from cur point
                float length = end - curr.z;

                // length from cur to next point
                float segmentLength = next.z - curr.z;

                if (curr.z <= end) {
                    float f = length / segmentLength;

                    _point = glm::vec2(curr) + (glm::vec2(next) - glm::vec2(curr)) * f;
                    _rotation = (glm::vec2(next) - glm::vec2(curr)) / segmentLength;

                    m_curAdvance = end;
                    return true;

                } else {
                    if (m_curPoint == 0) {
                        _point = glm::vec2(curr);
                        _rotation = (glm::vec2(next) - glm::vec2(curr)) / segmentLength;
                        m_curAdvance = 0;
                        return false;
                    }
                    m_curPoint -= 1;
                }
            }
        }

        return false;
    }

    float lengthToNextSegment() {
        if (m_curPoint >= m_points.size()-1) { return 0; }
        return m_points[m_curPoint + 1].z - m_curAdvance;
    }
    float lengthToPrevSegment() {
        if (m_curPoint >= m_points.size()) { return 0; }
        return m_curAdvance - m_points[m_curPoint].z;
    }

    LineSampler(Points _points) : m_points(_points) { }
    LineSampler() { }

private:
    Points m_points;

    size_t m_curPoint = 0;
    float m_curAdvance = 0.f;
};

}
