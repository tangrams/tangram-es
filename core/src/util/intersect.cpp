#include "intersect.h"
#include <cmath>

// Helper function for intersection tests; returns true iff [a0, a1] is disjoint from [b0, b1] (precondition: a0 < a1 && b0 < b1)
bool rangesDisjoint(float _a0, float _a1, float _b0, float _b1) {
    return _a1 < _b0 || _a0 > _b1;
}

bool AABBIntersectsTrapezoid(const glm::vec4& _aabb, const glm::vec2& _A, const glm::vec2& _B, const glm::vec2& _C, const glm::vec2& _D) {

    float box_min, box_max, trap_min, trap_max;
    float b0, b1, b2, b3, t0, t1, t2, t3;
    glm::vec2 norm;

    // Use the Separating Axis Theorem to check intersection

    // First, check axes of the bouding box, since these are simply the basis axes x and y
    
    // x
    box_min = _aabb.x;
    box_max = _aabb.z;
    trap_min = fminf(fminf(fminf(_A.x, _B.x), _C.x), _D.x);
    trap_max = fmaxf(fmaxf(fmaxf(_A.x, _B.x), _C.x), _D.x);

    if (rangesDisjoint(box_min, box_max, trap_min, trap_max)) {
        return false;
    }

    // y
    box_min = _aabb.y;
    box_max = _aabb.w;
    trap_min = fminf(fminf(fminf(_A.y, _B.y), _C.y), _D.y);
    trap_max = fmaxf(fmaxf(fmaxf(_A.y, _B.y), _C.y), _D.y);

    if (rangesDisjoint(box_min, box_max, trap_min, trap_max)) {
        return false;
    }

    // Next, check the axis parallel to AB and CD
    norm = glm::vec2(_B.y - _A.y, _A.x - _B.x);

    b0 = _aabb.x * norm.x + _aabb.y * norm.y;
    b1 = _aabb.z * norm.x + _aabb.y * norm.y;
    b2 = _aabb.z * norm.x + _aabb.w * norm.y;
    b3 = _aabb.x * norm.x + _aabb.w * norm.y;

    box_min = fminf(fminf(fminf(b0, b1), b2), b3);
    box_max = fmaxf(fmaxf(fmaxf(b0, b1), b2), b3);

    t0 = _A.x * norm.x + _A.y * norm.y;
    t1 = _C.x * norm.x + _C.y * norm.y;
    trap_min = fminf(t0, t1);
    trap_max = fmaxf(t0, t1);

    if (rangesDisjoint(box_min, box_max, trap_min, trap_max)) {
        return false;
    }

    // Finally, check the axes parallel to AC and BD

    // AC
    norm = glm::vec2(_C.y - _A.y, _A.x - _C.x);

    b0 = _aabb.x * norm.x + _aabb.y * norm.y;
    b1 = _aabb.z * norm.x + _aabb.y * norm.y;
    b2 = _aabb.z * norm.x + _aabb.w * norm.y;
    b3 = _aabb.x * norm.x + _aabb.w * norm.y;

    box_min = fminf(fminf(fminf(b0, b1), b2), b3);
    box_max = fmaxf(fmaxf(fmaxf(b0, b1), b2), b3);

    t0 = _A.x * norm.x + _A.y * norm.y;
    t1 = _B.x * norm.x + _B.y * norm.y;
    t2 = _C.x * norm.x + _C.y * norm.y;
    t3 = _D.x * norm.x + _D.y * norm.y;

    trap_min = fminf(fminf(fminf(t0, t1), t2), t3);
    trap_max = fmaxf(fmaxf(fmaxf(t0, t1), t2), t3);

    if (rangesDisjoint(box_min, box_max, trap_min, trap_max)) {
        return false;
    }

    // BD
    norm = glm::vec2(_D.y - _B.y, _B.x - _D.x);

    b0 = _aabb.x * norm.x + _aabb.y * norm.y;
    b1 = _aabb.z * norm.x + _aabb.y * norm.y;
    b2 = _aabb.z * norm.x + _aabb.w * norm.y;
    b3 = _aabb.x * norm.x + _aabb.w * norm.y;

    box_min = fminf(fminf(fminf(b0, b1), b2), b3);
    box_max = fmaxf(fmaxf(fmaxf(b0, b1), b2), b3);

    t0 = _A.x * norm.x + _A.y * norm.y;
    t1 = _B.x * norm.x + _B.y * norm.y;
    t2 = _C.x * norm.x + _C.y * norm.y;
    t3 = _D.x * norm.x + _D.y * norm.y;

    trap_min = fminf(fminf(fminf(t0, t1), t2), t3);
    trap_max = fmaxf(fmaxf(fmaxf(t0, t1), t2), t3);

    if (rangesDisjoint(box_min, box_max, trap_min, trap_max)) {
        return false;
    }

    // If none of these axes separate the bbox and trapezoid, then they intersect
    return true;

}
