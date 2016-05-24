#include "labelCollider.h"

#include "labels/labelSet.h"
#include "glm/gtc/matrix_transform.hpp"

#define TILE_SIZE 512
#define MAX_SCALE 2

namespace Tangram {

void LabelCollider::setup(float _tileScale) {

    m_tileScale = _tileScale * MAX_SCALE;

     // TODO use pixel scale
     m_screenSize = glm::vec2{ TILE_SIZE * MAX_SCALE * _tileScale };
}

void LabelCollider::addLabels(std::vector<std::unique_ptr<Label>>& _labels) {

    glm::mat4 mvp = glm::scale(glm::mat4(1.0), glm::vec3(m_tileScale));

    // Place tile centered
    // mvp[3][0] = -0.5f;
    // mvp[3][1] = -0.5f;

    for (auto& label : _labels) {

        if (label->canOcclude()) {
            label->update(mvp, m_screenSize, 1, true);

            m_aabbs.push_back(label->aabb());
            m_aabbs.back().m_userData = (void*)label.get();

            m_labels.push_back(label.get());
        }
    }
}

void LabelCollider::process() {

    m_isect2d.resize({m_screenSize.x / 256, m_screenSize.y / 256}, m_screenSize);

    m_isect2d.intersect(m_aabbs);

    std::sort(m_isect2d.pairs.begin(), m_isect2d.pairs.end(),
              [&](auto& a, auto& b) {
                  const auto& aabb1 = m_aabbs[a.first];
                  const auto& aabb2 = m_aabbs[b.first];

                  auto l1 = static_cast<Label*>(aabb1.m_userData);
                  auto l2 = static_cast<Label*>(aabb2.m_userData);

                  if (l1->options().priority != l2->options().priority) {
                      // lower numeric priority means higher priority
                      return l1->options().priority > l2->options().priority;
                  }
                  // just so it is consistent between two instances
                  return (l1->hash() < l2->hash());
              });

    // Narrow Phase, resolve conflicts
    for (auto& pair : m_isect2d.pairs) {
        const auto& aabb1 = m_aabbs[pair.first];
        const auto& aabb2 = m_aabbs[pair.second];

        auto l1 = static_cast<Label*>(aabb1.m_userData);
        auto l2 = static_cast<Label*>(aabb2.m_userData);


        if (l1->parent() && l1->parent()->isOccluded()) {
            l1->occlude();
        }
        if (l2->parent() && l2->parent()->isOccluded()) {
            l2->occlude();
        }

        if (l1->isOccluded() || l2->isOccluded()) {
            // One of this pair is already occluded.
            // => conflict solved
            continue;
        }

        if (!intersect(l1->obb(), l2->obb())) { continue; }

        if (l1->options().priority != l2->options().priority) {
            // lower numeric priority means higher priority
            if(l1->options().priority > l2->options().priority) {
                l1->occlude();
            } else {
                l2->occlude();
            }
        } else {
            // just so it is consistent between two instances
            if (l1->hash() < l2->hash()) {
                l1->occlude();
            } else {
                l2->occlude();
            }
        }
    }

    int cnt = 0;
    for (auto* label : m_labels) {

        // Manage link occlusion (unified icon labels)
        if (label->parent() && label->parent()->isOccluded()) {
            label->occlude();
        }

        if (label->isOccluded()) {
            cnt++;

            label->enterState(Label::State::dead, 0.0f);
        }

        // LOG("occluded: %d %f/%f ", label->isOccluded(),
        //     label->transform().state.screenPos.x,
        //     label->transform().state.screenPos.y);
    }

    LOG("Dropped %d/%d labels", cnt, m_labels.size());

    m_labels.clear();
    m_aabbs.clear();
}

}
