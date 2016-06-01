#pragma once

#include "isect2d.h"
#include "glm_vec.h" // for isect2d.h

#include <memory>
#include <vector>

namespace Tangram {

class Label;

class LabelCollider {

public:

    void setup(float _tileScale);

    void addLabels(std::vector<std::unique_ptr<Label>>& _labels);

    void process();

private:

    void handleRepeatGroup(size_t startPos);

    using AABB = isect2d::AABB<glm::vec2>;
    using OBB = isect2d::OBB<glm::vec2>;
    using CollisionPairs = std::vector<isect2d::ISect2D<glm::vec2>::Pair>;

    // Parallel vectors
    std::vector<Label*> m_labels;
    std::vector<AABB> m_aabbs;

    isect2d::ISect2D<glm::vec2> m_isect2d;

    float m_tileScale;

    glm::vec2 m_screenSize;
};

}
