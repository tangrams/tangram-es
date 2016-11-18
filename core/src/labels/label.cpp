#include "label.h"

#include "util/geom.h"
#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtx/norm.hpp"
#include "tangram.h"
#include "platform.h"
#include "view/view.h"
#include "log.h"
#include "tile/tile.h"
#include "util/mapProjection.h"

namespace Tangram {

const float Label::activation_distance_threshold = 2;

Label::Label(Label::WorldTransform _worldTransform, glm::vec2 _size, uint32_t _selectionColor, Type _type, Options _options)
    : m_state(State::none),
      m_selectionColor(_selectionColor),
      m_type(_type),
      m_worldTransform(_worldTransform),
      m_dim(_size),
      m_options(_options) {

    if (!m_options.collide || m_type == Type::debug) {
        enterState(State::visible, 1.0);
    } else {
        m_screenTransform.alpha = 0.0;
    }

    m_occludedLastFrame = false;
    m_occluded = false;
    m_parent = nullptr;
    m_anchorIndex = 0;
}

Label::~Label() {}

void Label::setParent(Label& _parent, bool _definePriority, bool _defineCollide) {
    m_parent = &_parent;

    if (_definePriority) {
        m_options.priority = _parent.options().priority + 0.5f;
    }

    if (_defineCollide) {
        m_options.collide = _parent.options().collide;
    }

    applyAnchor(m_options.anchors[m_anchorIndex]);
}

float Label::screenDistance2(glm::vec2 _screenPosition) const {
    return glm::length2(m_obb.getCentroid() - _screenPosition);
}

std::vector<LngLat> Label::coordinates(const Tile& _tile, const MapProjection& _projection) {
    std::vector<LngLat> coordinates;

    if (m_type == Type::line) {
        for (int i = 0; i < 2; ++i) {
            glm::vec2 tileCoord = glm::vec2(m_worldTransform.positions[i]);
            glm::dvec2 degrees = _tile.coordToLngLat(tileCoord, _projection);
            coordinates.push_back({degrees.x, degrees.y});
        }
    } else {
        glm::vec2 tileCoord = glm::vec2(m_worldTransform.position);
        glm::dvec2 degrees = _tile.coordToLngLat(tileCoord, _projection);
        coordinates.push_back({degrees.x, degrees.y});
    }

    return coordinates;
}

float Label::worldLineLength2() const {
    float worldLength2 = 0.f;

    if (m_type == Type::line) {
        worldLength2 = glm::length2(m_worldTransform.positions[0] - m_worldTransform.positions[1]);
    }

    return worldLength2;
}

bool Label::offViewport(const glm::vec2& _screenSize) {
    const auto& quad = m_obb.getQuad();

    for (int i = 0; i < 4; ++i) {
        if (m_options.flat) {
            if (m_screenTransform.positions[i].x < _screenSize.x
             && m_screenTransform.positions[i].x > 0
             && m_screenTransform.positions[i].y < _screenSize.y
             && m_screenTransform.positions[i].y > 0) {
                return false;
            }
        } else {
            if (quad[i].x < _screenSize.x && quad[i].x > 0 &&
                quad[i].y < _screenSize.y && quad[i].y > 0) {
                return false;
            }
        }
    }

    return true;
}

bool Label::canOcclude() {
    if (!m_options.collide) {
        return false;
    }

    int occludeFlags = (State::visible |
                        State::none |
                        State::skip_transition |
                        State::fading_in |
                        State::fading_out |
                        State::sleep |
                        State::out_of_screen |
                        State::dead);

    return (occludeFlags & m_state) && !(m_type == Type::debug);
}

bool Label::visibleState() const {
    int visibleFlags = (State::visible |
                        State::fading_in |
                        State::fading_out |
                        State::skip_transition);

    return (visibleFlags & m_state);
}

void Label::skipTransitions() {
    enterState(State::skip_transition, 0.0);
}

glm::vec2 Label::center() const {
    return m_obb.getCentroid();
}

void Label::enterState(const State& _state, float _alpha) {
    if (m_state == State::dead) { return; }

    m_state = _state;
    setAlpha(_alpha);

    if (m_state == State::sleep) {
        // Reset anchor fallback index
        m_anchorIndex = 0;
    }

}

void Label::setAlpha(float _alpha) {
    m_screenTransform.alpha = CLAMP(_alpha, 0.0, 1.0);
}

void Label::resetState() {

    if (m_state == State::dead) { return; }

    m_occludedLastFrame = false;
    m_occluded = false;
    m_anchorIndex = 0;
    enterState(State::none, 0.0);
}

void Label::print() const {
    LOG("Label - %p", this);
    LOG("\tm_occludedLastFrame: %d", m_occludedLastFrame);
    LOG("\tm_occluded: %d", m_occluded);
    std::string state;
    switch (m_state) {
        case State::none: state = "none"; break;
        case State::visible: state = "visible"; break;
        case State::fading_in: state = "fading_in"; break;
        case State::fading_out: state = "fading_out"; break;
        case State::skip_transition: state = "skip_transition"; break;
        case State::sleep: state = "sleep"; break;
        case State::dead: state = "dead"; break;
        case State::out_of_screen: state = "out_of_screen"; break;
    }
    LOG("\tm_state: %s", state.c_str());
    LOG("\tm_anchorIndex: %d", m_anchorIndex);
    LOG("\tscreenPos: %f/%f", m_screenTransform.position.x, m_screenTransform.position.y);

}

bool Label::nextAnchor() {
    int index = m_anchorIndex;
    setAnchorIndex((index + 1) % m_options.anchors.count);

    return m_anchorIndex != index;
}

bool Label::setAnchorIndex(int _index) {
    if (_index >= int(m_options.anchors.count) || _index < 0) {
        return false;
    }
    m_anchorIndex = _index;

    applyAnchor(m_options.anchors[m_anchorIndex]);

    return true;
}

bool Label::update(const glm::mat4& _mvp, const ViewState& _viewState, bool _drawAllLabels) {

    m_occludedLastFrame = m_occluded;
    m_occluded = false;

    if (m_state == State::dead) {
        if (_drawAllLabels) {
            m_occluded = true;
        } else {
            return false;
        }
    }

    bool ruleSatisfied = updateScreenTransform(_mvp, _viewState, _drawAllLabels);

    // one of the label rules has not been satisfied
    if (!ruleSatisfied) {
        enterState(State::sleep, 0.0);
        return false;
    }

    // update the view-space bouding box
    updateBBoxes(_viewState.fractZoom);

    // checks whether the label is out of the viewport
    if (offViewport(_viewState.viewportSize)) {
        enterState(State::out_of_screen, 0.0);
        if (m_occludedLastFrame) {
            m_occluded = true;
            return false;
        }
    } else if (m_state == State::out_of_screen) {
        enterState(State::sleep, 0.0);
    }

    return true;
}

bool Label::evalState(float _dt) {

#ifdef DEBUG
    if (Tangram::getDebugFlag(DebugFlags::draw_all_labels)) {
        enterState(State::visible, 1.0);
        return false;
    }
#endif

    switch (m_state) {
        case State::none:
        case State::sleep:
            if (m_occluded) {
                enterState(State::sleep, 0.0);
                return false;
            }
            if (m_options.showTransition.time == 0.f) {
                enterState(State::visible, 1.0);
                return false;
            }
            m_fade.reset(true, m_options.showTransition.ease,
                         m_options.showTransition.time);
            enterState(State::fading_in, 0.0);
            return true;

        case State::visible:
            if (!m_occluded) { return false; }

            if (m_options.hideTransition.time == 0.f) {
                enterState(State::sleep, 0.0);
                return false;
            }
            m_fade.reset(false, m_options.hideTransition.ease,
                         m_options.hideTransition.time);

            enterState(State::fading_out, 1.0);
            return true;

        case State::fading_in:
            if (m_occluded) {
                enterState(State::sleep, 0.0);
                return false;
            }
            setAlpha(m_fade.update(_dt));
            if (m_fade.isFinished()) {
                enterState(State::visible, 1.0);
                return false;
            }
            return true;

        case State::fading_out:
            // if (!m_occluded) {
            //     enterState(State::fading_in, m_transform.state.alpha);
            //     m_fade.reset(false, m_options.hideTransition.ease,
            //                  m_options.showTransition.time);
            //     return true;
            // }
            setAlpha(m_fade.update(_dt));
            if (m_fade.isFinished()) {
                enterState(State::sleep, 0.0);
                return false;
            }
            return true;

        case State::skip_transition:
            if (m_occluded) {
                enterState(State::sleep, 0.0);
            } else {
                enterState(State::visible, 1.0);
            }
            return true;

        case State::dead:
        case State::out_of_screen:
            break;
    }

    return false;
}

}
