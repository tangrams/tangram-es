#pragma once

namespace Tangram {

struct ScreenTransform {

    struct Buffer {
        std::vector<glm::vec3> points;
        std::vector<glm::vec2> path;
        void clear() {
            points.clear();
            path.clear();
        }
    };

    ScreenTransform(Buffer& _transformBuffer, Range& _range, bool _initRange = false)
        : points(_transformBuffer.points),
          m_scratchBuffer(_transformBuffer.path),
          range(_range) {
        if (_initRange) {
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

    auto& scratchBuffer() {
        m_scratchBuffer.clear();
        return m_scratchBuffer;
    }

private:
    std::vector<glm::vec3>& points;
    std::vector<glm::vec2>& m_scratchBuffer;
    Range& range;
};

}
