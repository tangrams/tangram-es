#pragma once

#include "glm/glm.hpp"

namespace Tangram {

struct LineSamplerPoint {
    LineSamplerPoint(glm::vec2 _coord, float _length) : coord(_coord), length(_length) {}
    glm::vec2 coord;
    float length;
};

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
            //glm::vec2 p = { _points[i].x, _points[i].y };
            glm::vec2 n = { _points[i+1].x, _points[i+1].y };
            float d = glm::distance(p, n);
            //if (d > 0.f) {
            sum += d;
            m_points.push_back({{n.x, n.y}, sum});
            //}
            p = n;
        }
        reset();
    }

    template<typename T>
    void add(T _point) {

        glm::vec2 p = { _point.x, _point.y };

        if (m_points.empty()) {
            m_points.push_back({p, 0.f});
            return;
        }

        size_t i = m_points.size()-1;

        glm::vec2 prev = { m_points[i].coord.x, m_points[i].coord.y };
        float d = glm::distance(prev, p);

        m_points.push_back({p, m_points[i].length + d});
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
        for (auto& p : m_points) { p.length = sum - p.length; }

        m_curAdvance = 0.f;
        m_curPoint = 0;
    }

    float sumLength() {
        if (m_points.empty()) { return 0.f; }

        return m_points[m_points.size()-1].length;
    }

    size_t curSegment() {
        return m_curPoint;
    }

    LineSamplerPoint point(size_t _pos) {
        return m_points[_pos];
    }

    float segmentLength(size_t _pos) {
        if (_pos >= m_points.size()-1) { return 0; }

        return (m_points[_pos+1].length - m_points[_pos].length);
    }

    glm::vec2 segmentDirection(size_t _pos) {
        if (_pos >= m_points.size()-1) { return {}; }

        return (m_points[_pos+1].coord - m_points[_pos].coord) /
            (m_points[_pos+1].length - m_points[_pos].length);
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
                float length = end - curr.length;
                // length from cur to next point
                float segmentLength = next.length - curr.length;

                if (length <= segmentLength) {
                    float f = length / segmentLength;

                    _point = curr.coord + (next.coord - curr.coord) * f;
                    _rotation = (next.coord - curr.coord) / segmentLength;

                    m_curAdvance = end;
                    return true;

                } else {
                    if (m_curPoint >= m_points.size()-2) {
                        _point = next.coord;
                        _rotation = (next.coord - curr.coord) / segmentLength;
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
                float length = end - curr.length;

                // length from cur to next point
                float segmentLength = next.length - curr.length;

                if (curr.length <= end) {
                    float f = length / segmentLength;

                    _point = curr.coord + (next.coord - curr.coord) * f;
                    _rotation = (next.coord - curr.coord) / segmentLength;

                    m_curAdvance = end;
                    return true;

                } else {
                    if (m_curPoint == 0) {
                        _point = curr.coord;
                        _rotation = (next.coord - curr.coord) / segmentLength;

                        m_curAdvance = 0;
                        return false;
                    }
                    m_curPoint -= 1;
                }
            }
        }

        return false;
    }

    LineSampler(Points _points) : m_points(_points) { }
    LineSampler() { }
private:
    Points m_points;

    size_t m_curPoint = 0;
    float m_curAdvance = 0.f;

};

}
