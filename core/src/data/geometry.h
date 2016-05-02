#pragma once

#include <vector>
#include "glm/vec3.hpp"

namespace Tangram {

enum GeometryType {
    unknown,
    points,
    lines,
    polygons
};

typedef glm::vec3 Point;

template<typename T>
class Geometry {

public:

    static Geometry line(const std::vector<T>& _line) {
        Geometry geom;
        geom.m_points = _line;
        geom.m_lines = {_line.size()};
        return geom;
    }

    static Geometry polygon(const std::vector<std::vector<T>>& _poly) {
        Geometry geom;
        // geom.m_points = _line;
        // geom.m_lines = {_line.size()};
        return geom;
    }

    void clear()  {
        m_points.clear();
        m_lines.clear();
        m_polys.clear();
    }

    /*
     * Add points to Geometry with addPoint(), then use endLine() or endRing()
     * to finish a ring or line element.
     */
    void addPoint(const T& _point) {
        m_points.push_back(_point);
    }

    /* Finish line with all the added points (since any previous call to endLine) */
    void endLine() {
        if (m_points.empty() || (!m_lines.empty() && m_lines.back() == m_points.size())) {
            //LOGW("No Points were added to Line!");
            return;
        }
        m_lines.push_back(m_points.size());
    }

    /* Finish polygon ring with all the added points (since any previous call to endRing) */
    void endRing() {
        endLine();
    }

    /* Finish polygon with all the added rings (since any previous call to endPoly) */
    void endPoly() {
        if (m_lines.empty() || (!m_polys.empty() && m_polys.back() == m_lines.size())) {
            //LOGW("No rings were added to Polygon!");
            return;
        }
        m_polys.push_back(m_lines.size());
    }

    const std::vector<T>& points() const { return m_points; }
    std::vector<T>& points() { return m_points; }
    std::vector<size_t>& polyIndex() { return m_polys; }
    std::vector<size_t>& lineIndex() { return m_lines; }

    struct point_range {
        using const_iterator = typename std::vector<T>::const_iterator;

        typedef T value_type;

        point_range(const Geometry& _geom, size_t _begin, size_t _end)
            : m_begin(_geom.m_points.begin() + _begin),
              m_end(_geom.m_points.begin() + _end) {}

        point_range(const_iterator _begin, const_iterator _end)
            : m_begin(_begin),
              m_end(_end) {}

        point_range(point_range _line, size_t _begin, size_t _end)
            : m_begin(_line.m_begin + _begin),
              m_end(_line.m_begin + _end) {}

        const_iterator begin() { return m_begin; }
        const_iterator end() { return m_end; }

        const T& operator[](size_t pos) const {
            return *(m_begin + pos);
        }

        size_t size() const { return m_end - m_begin; }

    private:
        const_iterator m_begin, m_end;
    };

    // Iterate through lines/rings
    struct line_iterator {

        line_iterator(const Geometry& _geom, size_t _pos)
            : m_line(_geom,
                     _pos == 0 ? 0 : _geom.m_lines[_pos-1],
                     _geom.m_lines[_pos]),
              m_geom(_geom),
              m_pos(_pos) {}

        line_iterator& operator++() {
            m_pos++;

            m_line = { m_geom,
                       m_geom.m_lines[m_pos-1],
                       m_geom.m_lines[m_pos] };

            return *this;
        }

        point_range& operator*() {
            return m_line;
        }
        point_range* operator->() {
            return &m_line;
        }

        bool operator==(const line_iterator& rhs) const {
            return m_pos == rhs.m_pos;
        }

        bool operator!=(const line_iterator& rhs) const {
            return m_pos != rhs.m_pos;
        }

    private:
        point_range m_line;
        const Geometry& m_geom;
        size_t m_pos;
    };

    // MultiLineString / Polygon rings
    struct line_range {
        typedef point_range value_type;

        line_range(const Geometry& _geom, size_t _beginLine, size_t _endLine)
            : m_geom(_geom),
              m_begin(_beginLine),
              m_end(_endLine) {}

        line_range(const Geometry& _geom)
            : m_geom(_geom),
              m_begin(0),
              m_end(0) {}

        line_iterator begin() const {
            return { m_geom, m_begin };
        }

        line_iterator end() const {
            return { m_geom, m_end } ;
        }

        point_range operator[](size_t pos) const {
            size_t begin = 0;
            if (m_begin + pos > 0) {
                begin = m_geom.m_lines[m_begin + pos-1];
            }
            size_t end = m_geom.m_lines[m_begin + pos];

            return {
                m_geom.m_points.begin() + begin,
                m_geom.m_points.begin() + end
            };
        }

        size_t size() const { return m_end - m_begin; }

        bool empty() const { return (m_end - m_begin) == 0; }

        const Point& pointAt(size_t _pos) const {
            if (m_begin == 0) {
                return m_geom.m_points[_pos];
            } else {
                return m_geom.m_points[m_geom.m_lines[m_begin-1] + _pos];
            }
        }

        point_range points() const {
            size_t begin = 0;
            if (m_begin > 0) {
                begin = m_geom.m_lines[m_begin - 1];
            }
            size_t end = m_geom.m_lines[m_end];
            return {
                m_geom.m_points.begin() + begin,
                m_geom.m_points.begin() + end
            };
        }

        void setPolygon(size_t _pos) {
            if (m_geom.m_polys.empty()) {
                m_begin = 0;
                m_end = 0;
            } else {
                m_begin = _pos == 0 ? 0 : m_geom.m_polys[_pos-1];
                m_end = m_geom.m_polys[_pos];
            }
        }

    private:
        const Geometry& m_geom;
        size_t m_begin;
        size_t m_end;
    };

    // Iterate through polygons
    struct poly_iterator {

        poly_iterator(const Geometry& _geom, size_t _pos)
            : m_rings(_geom),
              m_pos(_pos) {
            m_rings.setPolygon(m_pos);
        }

        poly_iterator& operator++() {
            m_pos++;
            m_rings.setPolygon(m_pos);
            return *this;
        }

        line_range& operator*() {
            return m_rings;
        }

        line_range* operator->() {
            return &m_rings;
        }

        bool operator==(const poly_iterator& rhs) const {
            return m_pos == rhs.m_pos;
        }

        bool operator!=(const poly_iterator& rhs) const {
            return m_pos != rhs.m_pos;
        }

    private:
        line_range m_rings;
        size_t m_pos;
    };

    // MultiPolygon
    struct poly_range {
        typedef line_range value_type;

        poly_range(const Geometry& _geom, size_t _begin, size_t _end)
            : m_geom(_geom),
              m_begin(_begin),
              m_end(_end) {}

        poly_iterator begin() const {
            return { m_geom, m_begin };
        }

        poly_iterator end() const {
            return { m_geom, m_end };
        }

        line_range operator[](size_t pos) const {
            return { m_geom, pos };
        }
        size_t size() const { return m_end - m_begin; }

    private:
        const Geometry& m_geom;
        size_t m_begin;
        size_t m_end;
    };

    line_range lines() const {
        return {*this, 0, m_lines.size()};
    };

    poly_range polygons() const {
        return {*this, 0, m_polys.size()};
    };

    GeometryType type = GeometryType::unknown;

private:
    std::vector<T> m_points = {};
    std::vector<size_t> m_lines = {};
    std::vector<size_t> m_polys = {};

};

using Line = Geometry<Point>::point_range;
using Polygon = Geometry<Point>::line_range;

}
