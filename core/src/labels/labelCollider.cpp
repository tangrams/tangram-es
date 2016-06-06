#include "labelCollider.h"

#include "labels/labelSet.h"
#include "glm/gtc/matrix_transform.hpp"

#define MAX_SIZE 512

namespace Tangram {

const glm::vec2 screen_size{ MAX_SIZE*2, MAX_SIZE*2};
const int tile_size = MAX_SIZE;

void LabelCollider::addLabels(std::vector<std::unique_ptr<Label>>& _labels) {

    float scale = 1;

    glm::mat4 mvp = glm::scale(glm::mat4(1.0), glm::vec3(scale));

    // Place tile centered
    mvp[3][0] = -0.5f;
    mvp[3][1] = -0.5f;

    for (auto& label : _labels) {

        if (label->canOcclude()) {
            label->update(mvp, screen_size, 1, true);

            m_aabbs.push_back(label->aabb());
            m_aabbs.back().m_userData = (void*)label.get();

            m_labels.push_back(label.get());
        }
    }
}

void LabelCollider::process() {

    m_isect2d.resize({tile_size / 256, tile_size / 256}, {tile_size, tile_size});

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
        if (label->parent() && (label->parent()->isOccluded() || !label->parent()->visibleState())) {
            label->occlude();
        }

        if (label->isOccluded()) { cnt++; }


        LOG("occlued: %d %f/%f ", label->isOccluded(),
            label->transform().modelPosition1.x,
            label->transform().modelPosition1.y);

        label->evalState(screen_size, 0);
    }

    LOG("Dropped %d/%d labels", cnt, m_labels.size());

    m_labels.clear();
    m_aabbs.clear();
}

}
