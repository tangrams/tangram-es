#pragma once

#include "labels/label.h"
#include "labels/screenTransform.h"
#include "util/mapProjection.h"
#include "util/types.h"

#include "isect2d.h"
#include "glm_vec.h" // for isect2d.h
#include <memory>
#include <vector>

namespace Tangram {

class Label;
struct ViewState;

class LabelCollider {

public:


    void addLabels(std::vector<std::unique_ptr<Label>>& _labels);

    void process(TileID _tileID, float _tileInverseScale, float _tileSize);

private:

    void handleRepeatGroup(size_t startPos);

    using AABB = isect2d::AABB<glm::vec2>;
    using OBB = isect2d::OBB<glm::vec2>;
    using CollisionPairs = std::vector<isect2d::ISect2D<glm::vec2>::Pair>;

    struct LabelEntry {

        LabelEntry(Label* _label)
            : label(_label),
              priority(_label->options().priority) {}

        Label* label;

        float priority;

        Range obbs;
        Range transform;
    };

    // Parallel vectors

    std::vector<LabelEntry> m_labels;
    std::vector<AABB> m_aabbs;
    std::vector<OBB> m_obbs;

    isect2d::ISect2D<glm::vec2> m_isect2d;

    ScreenTransform::Buffer m_transforms;
};

}
