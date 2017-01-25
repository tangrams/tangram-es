#pragma once

#include "glm/vec2.hpp"

namespace Tangram {

// ScreenTransform is a view into ScreenTransform::Buffer
struct ScreenTransform {

    // ScreenTransform::Buffer holds screen space coordinates of multiple labels
    // (used by LabelCollider and Labels class)
    struct Buffer {
        std::vector<glm::vec3> points;
        std::vector<glm::vec2> path;
        void clear() {
            points.clear();
            path.clear();
        }
    };

    ScreenTransform(Buffer& _transformBuffer, Range& _range)
        : buffer(_transformBuffer),
          points(_transformBuffer.points),
          range(_range) {

        if (!range.length) {
            range.start = points.size();
        }
    }

    auto begin() { return points.begin() + range.start; }
    auto end() { return points.begin() + range.end(); }

    bool empty() const { return range.length == 0; }
    size_t size() const { return range.length; }
    void clear() {
        range.length = 0;
        points.resize(range.start);
    }

    auto operator[](size_t _pos) const { return points[range.start + _pos]; }

    void push_back(glm::vec3 _p) {
        points.push_back(_p);
        range.length += 1;
    }

    void push_back(glm::vec2 _p) {
        points.emplace_back(_p, 0);
        range.length += 1;
    }

    // Get a temporary coordinate buffer (used for curved labels line smoothing)
    std::vector<glm::vec2>& scratchBuffer() {
        buffer.path.clear();
        return buffer.path;
    }

private:
    Buffer &buffer;
    std::vector<glm::vec3>& points;
    Range& range;
};

}
