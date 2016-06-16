#include "labelCollider.h"

#include "labels/labelSet.h"
#include "glm/gtc/matrix_transform.hpp"

#define MAX_SCALE 2

namespace Tangram {

void LabelCollider::setup(float _tileSize, float _tileScale) {

    // Maximum scale at which this tile is used (unless it's a proxy)
    m_tileScale = _tileScale * MAX_SCALE;

    // TODO use pixel scale
    m_screenSize = glm::vec2{ _tileSize * m_tileScale };
}

void LabelCollider::addLabels(std::vector<std::unique_ptr<Label>>& _labels) {

    for (auto& label : _labels) {
        if (label->canOcclude()) {
            m_labels.push_back(label.get());
        }
    }
}

void LabelCollider::handleRepeatGroup(size_t startPos) {

    float threshold2 = pow(m_labels[startPos]->options().repeatDistance, 2);
    size_t repeatGroup = m_labels[startPos]->options().repeatGroup;

    // Get the range
    size_t endPos = startPos;
    for (;startPos > 0; startPos--) {
        if (m_labels[startPos-1]->options().repeatGroup != repeatGroup) { break; }
    }
    for (;endPos < m_labels.size()-1; endPos++) {
        if (m_labels[endPos+1]->options().repeatGroup != repeatGroup) { break; }
    }


    for (size_t i = startPos; i < endPos; i++) {
        Label* l1 = m_labels[i];
        if (l1->isOccluded()) { continue; }

        for (size_t j = i+1; j <= endPos; j++) {
            Label* l2 = m_labels[j];
            if (l2->isOccluded()) { continue; }

            float d2 = distance2(l1->center(), l2->center());
            if (d2 < threshold2) {
                l2->occlude();
            }
        }
    }
}

void LabelCollider::process() {

    // Sort labels so that all labels of one repeat group are next to each other
    std::sort(m_labels.begin(), m_labels.end(),
              [](auto* l1, auto* l2) {
                  if (l1->options().priority != l2->options().priority) {
                      // lower numeric priority means higher priority
                      return l1->options().priority > l2->options().priority;
                  }
                  if (l1->options().repeatGroup != l2->options().repeatGroup) {
                      return l1->options().repeatGroup < l2->options().repeatGroup;
                  }

                  if (l1->type() == Label::Type::line && l2->type() == Label::Type::line) {
                      // Prefer the label with longer line segment as it has a chance
                      // to be shown earlier (also on the lower zoom-level)
                      // TODO compare fraction segment_length/label_width
                      return glm::length2(l1->transform().modelPosition1 - l1->transform().modelPosition2) >
                             glm::length2(l2->transform().modelPosition1 - l2->transform().modelPosition2);
                  }

                  return l1->hash() < l2->hash();
              });

    // Project tile to NDC (-1 to 1, y-up)
    glm::mat4 mvp{1};
    // Scale tile to 'fullscreen'
    mvp[0][0] = 2;
    mvp[1][1] = -2;
    // Place tile centered
    mvp[3][0] = -1;
    mvp[3][1] = 1;

    for (auto* label : m_labels) {
        label->update(mvp, m_screenSize, 1, true);

         m_aabbs.push_back(label->aabb());
    }

    m_isect2d.resize({m_screenSize.x / 128, m_screenSize.y / 128}, m_screenSize);

    m_isect2d.intersect(m_aabbs);

    // Set the first item to be the one with higher priority
    for (auto& pair : m_isect2d.pairs) {

        auto* l1 = m_labels[pair.first];
        auto* l2 = m_labels[pair.second];

        if (l1->options().priority > l2->options().priority) {
            std::swap(pair.first, pair.second);

            // Note: Mark the label to be potentially occluded
            l2->enterState(Label::State::sleep, 0.0f);
        } else {
            l1->enterState(Label::State::sleep, 0.0f);
        }
    }

    // Sort by priority on the first item
    std::sort(m_isect2d.pairs.begin(), m_isect2d.pairs.end(),
              [&](auto& a, auto& b) {
                  auto* l1 = m_labels[a.first];
                  auto* l2 = m_labels[b.first];

                  if (l1->options().priority != l2->options().priority) {
                      // lower numeric priority means higher priority
                      return l1->options().priority > l2->options().priority;
                  }

                  if (l1->type() == Label::Type::line &&
                      l2->type() == Label::Type::line) {
                      // Prefer the label with longer line segment as it has a chance
                      // to be shown earlier (also on the lower zoom-level)
                      // TODO compare fraction segment_length/label_width

                      return glm::length2(l1->transform().modelPosition1 - l1->transform().modelPosition2) >
                             glm::length2(l2->transform().modelPosition1 - l2->transform().modelPosition2);

                  }
                  // just so it is consistent between two instances
                  return (l1->hash() < l2->hash());
              });

    // The collision pairs are sorted in a way that:
    // - The first item may occlude the second it (but not the other way round!)
    // At each iteration where the priority decreases:
    // - the first item of the collision pair has a higher priority
    // - all items of following collision pairs have a lower priority
    // -> all labels of repeatGroups with higher priority have been added
    //    when reaching a collision pair with lower priority
    // This allows to remove repeated labels before they occlude other candidates

    size_t repeatGroup = 0;

    // Narrow Phase, resolve conflicts
    for (auto& pair : m_isect2d.pairs) {

        auto* l1 = m_labels[pair.first];
        auto* l2 = m_labels[pair.second];

        // Occlude labels within repeat group so that they don't occlude other labels
        if (repeatGroup != l1->options().repeatGroup) {
            repeatGroup = l1->options().repeatGroup;

            if (repeatGroup != 0) {
                handleRepeatGroup(pair.first);
            }
        }

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

        if (!intersect(l1->obb(), l2->obb())) {
            continue;
        }

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

    for (auto* label : m_labels) {

        // Manage link occlusion (unified icon labels)
        if (label->parent() && label->parent()->isOccluded()) {
            label->occlude();
        }

        if (label->isOccluded()) {
            label->enterState(Label::State::dead, 0.0f);
        } else {
            label->enterState(Label::State::wait_occ, 0.0f);
        }
    }

    m_labels.clear();
    m_aabbs.clear();
}

}
