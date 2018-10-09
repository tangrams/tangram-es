#include "labels/labelCollider.h"

#include "labels/curvedLabel.h"
#include "labels/labelSet.h"
#include "labels/obbBuffer.h"
#include "view/view.h" // ViewState

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/norm.hpp"

namespace Tangram {

void LabelCollider::addLabels(std::vector<std::unique_ptr<Label>>& _labels) {

    for (auto& label : _labels) {
        if (label->canOcclude()) {
            m_labels.push_back(label.get());
        }
    }
}

size_t LabelCollider::filterRepeatGroups(size_t startPos, size_t curPos) {

    size_t endGroup = m_labels[curPos].label->options().repeatGroup;
    size_t endPos = curPos;

    for (size_t pos = startPos; pos <= curPos; pos = endPos + 1) {
        size_t repeatGroup = m_labels[pos].label->options().repeatGroup;
        float  threshold2 = pow(m_labels[pos].label->options().repeatDistance, 2);

        // Find end of the current repeatGroup
        endPos = pos;

        for (;endPos < m_labels.size()-1; endPos++) {
            if (m_labels[endPos+1].label->options().repeatGroup != repeatGroup) {
                break;
            }
        }

        for (size_t i = pos; i < endPos; i++) {
            Label* l1 = m_labels[i].label;
            if (l1->isOccluded()) { continue; }

            for (size_t j = i+1; j <= endPos; j++) {
                Label* l2 = m_labels[j].label;
                if (l2->isOccluded()) { continue; }

                float d2 = distance2(l1->screenCenter(), l2->screenCenter());
                if (d2 < threshold2) {
                    l2->occlude();
                }
            }
        }
        if (repeatGroup == endGroup) { break; }
    }
    return endPos;
}

void LabelCollider::process(TileID _tileID, float _tileInverseScale, float _tileSize) {

    // Sort labels so that all labels of one repeat group are next to each other
    std::sort(m_labels.begin(), m_labels.end(),
              [](auto& e1, auto& e2) {
                  auto* l1 = e1.label;
                  auto* l2 = e2.label;

                  if (l1->options().priority != l2->options().priority) {
                      // lower numeric priority means higher priority
                      return l1->options().priority < l2->options().priority;
                  }
                  if (l1->options().repeatGroup != l2->options().repeatGroup) {
                      return l1->options().repeatGroup < l2->options().repeatGroup;
                  }

                  if (l1->type() == l2->type()) {
                      return l1->candidatePriority() < l2->candidatePriority();
                  }

                  return l1->hash() < l2->hash();
              });

    // Set view parameters so that the tile is rendererd at
    // style-zoom-level + 2. (scaled up by factor 4). This
    // filters out labels that are unlikely to become visible
    // within the tiles zoom-range.
    int overzoom = 2;
    float tileScale = pow(2, _tileID.s - _tileID.z + overzoom);
    glm::vec2 screenSize{ _tileSize * tileScale };

    // Project tile to NDC (-1 to 1, y-up)
    glm::mat4 mvp{1};
    mvp[0][0] = 1.0f;
    mvp[1][1] = -1.f;
    // Place tile centered
    mvp[3][0] = -0.5f;
    mvp[3][1] = 0.5f;

    ViewState viewState {
        false, // changedOnLastUpdate (unused)
        glm::dvec2{}, // center (unused)
        0.f, // zoom (unused)
        powf(2.f, _tileID.s + overzoom), // zoomScale
        0.f, // fractZoom
        screenSize, // viewPortSize
        _tileSize, // screenTileSize
    };

    m_obbs.clear();
    m_transforms.clear();

    for (auto it = m_labels.begin(); it != m_labels.end(); ) {
        auto& entry = *it;
        auto* label = entry.label;
        ScreenTransform transform { m_transforms, entry.transform };
        if (label->updateScreenTransform(mvp, viewState, nullptr, transform)) {

            OBBBuffer obbs { m_obbs, entry.obbs };

            label->obbs(transform, obbs);

            auto aabb = m_obbs[entry.obbs.start].getExtent();
            for (int i = entry.obbs.start+1; i < entry.obbs.end(); i++) {
                aabb = unionAABB(aabb, m_obbs[i].getExtent());
            }

            m_aabbs.push_back(aabb);
            it++;
        } else {
            it = m_labels.erase(it);
        }
    }

    if (m_labels.empty()) { return; }

    m_isect2d.resize({screenSize.x / 128, screenSize.y / 128}, screenSize);

    m_isect2d.intersect(m_aabbs);

    // Set the first item to be the one with higher priority
    for (auto& pair : m_isect2d.pairs) {

        auto& e1 = m_labels[pair.first];
        auto& e2 = m_labels[pair.second];
        auto* l1 = e1.label;
        auto* l2 = e2.label;

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

                  if (a.first == b.first) { return a.second < b.second; }

                  auto* l1 = m_labels[a.first].label;
                  auto* l2 = m_labels[b.first].label;

                  if (l1->options().priority != l2->options().priority) {
                      // lower numeric priority means higher priority
                      return l1->options().priority < l2->options().priority;
                  }

                  // Required ordering for 'filterRepeatGroups()'
                  if (l1->options().repeatGroup != l2->options().repeatGroup) {
                      return l1->options().repeatGroup < l2->options().repeatGroup;
                  }

                  if (l1->type() == l2->type() &&
                      l1->candidatePriority() != l2->candidatePriority()) {
                      return l1->candidatePriority() < l2->candidatePriority();
                  }

                  if (l1->hash() != l2->hash()) {
                      return (l1->hash() < l2->hash());
                  }

                  // just so it is consistent between two instances
                  return a.first < b.first;
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
    size_t lastFilteredLabelIndex = 0;

    // Narrow Phase, resolve conflicts
    for (auto& pair : m_isect2d.pairs) {

        auto& e1 = m_labels[pair.first];
        auto& e2 = m_labels[pair.second];
        auto* l1 = e1.label;
        auto* l2 = e2.label;

        // Occlude labels within repeat group so that they don't occlude other labels
        if (repeatGroup != l1->options().repeatGroup) {
            repeatGroup = l1->options().repeatGroup;

            if (repeatGroup != 0) {
                lastFilteredLabelIndex = filterRepeatGroups(lastFilteredLabelIndex, pair.first);
            }
        }

        // Dont let relatives occlude their child
        if (l1->relative() == l2 || l2->relative() == l1) {
            continue;
        }
        if (l1->isChild() && l1->relative()->isOccluded()) {
            l1->occlude();
        }
        if (l2->isChild() && l2->relative()->isOccluded()) {
            l2->occlude();
        }

        if (l1->isOccluded() || l2->isOccluded()) {
            // One of this pair is already occluded.
            // => conflict solved
            continue;
        }

        bool intersection = false;
        for (int i = e1.obbs.start; i < e1.obbs.end(); i++) {
            for (int j = e2.obbs.start; j < e2.obbs.end(); j++) {

                if (intersect(m_obbs[i], m_obbs[j])) {
                    intersection = true;

                    // break out of outer loop
                    i = e1.obbs.end();
                    break;
                }
            }
        }
        if (!intersection) { continue; }

        if (l1->options().priority != l2->options().priority) {
            // lower numeric priority means higher priority
            if(l1->options().priority > l2->options().priority) {
                l1->occlude();
            } else {
                l2->occlude();
            }
        } else {
            if (l1->type() == l2->type()) {
                if (l1->candidatePriority() > l2->candidatePriority()) {
                    l1->occlude();
                } else {
                    l2->occlude();
                }
            }

            // just so it is consistent between two instances
            if (l1->hash() < l2->hash()) {
                l1->occlude();
            } else {
                l2->occlude();
            }
        }
    }

    filterRepeatGroups(lastFilteredLabelIndex, m_labels.size()-1);

    for (auto& entry : m_labels) {
        auto* label = entry.label;

        // Manage link occlusion (unified icon labels)
        if (label->relative()) {
            // First check if the child is required is occluded
            if (label->relative()->isOccluded()) {
                label->occlude();
            } else if (!label->options().optional && label->isOccluded()) {
                label->relative()->occlude();
                label->relative()->enterState(Label::State::dead, 0.0f);
            }
        }

        if (label->isOccluded()) {
            label->enterState(Label::State::dead, 0.0f);
        } else {
            label->enterState(Label::State::none, 0.0f);
        }
    }

    m_labels.clear();
    m_aabbs.clear();
}

}
