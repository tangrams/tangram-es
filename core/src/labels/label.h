#pragma once

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include <climits> // needed in aabb.h
#include "aabb.h"
#include "obb.h"
#include "fadeEffect.h"
#include "util/types.h"
#include "util/hash.h"
#include "labels/labelProperty.h"
#include "tangram.h"

#include <string>
#include <limits>
#include <memory>

namespace Tangram {

struct ViewState;
class Tile;
class MapProjection;

class Label {

public:

    using OBB = isect2d::OBB<glm::vec2>;
    using AABB = isect2d::AABB<glm::vec2>;

    enum class Type {
        point,
        line,
        debug,
    };

    enum State {
        none            = 1 << 0,
        fading_in       = 1 << 1,
        fading_out      = 1 << 2,
        visible         = 1 << 3,
        sleep           = 1 << 4,
        out_of_screen   = 1 << 5,
        skip_transition = 1 << 6,
        dead            = 1 << 7,
    };

    struct WorldTransform {
        WorldTransform(glm::vec3 _wp) : position(_wp) {}
        WorldTransform(glm::vec2 _wp0, glm::vec2 _wp1) {
            positions[0] = _wp0;
            positions[1] = _wp1;
        }

        union {
            glm::vec3 position;     // The label position if the label is not a line
                                    // position.z stores the zoom-level
            glm::vec2 positions[2]; // The label positions if the label is a line
        };
    };

    struct ScreenTransform {
        ScreenTransform() {}
        union {
            glm::vec2 position;     // The label position if the label is not flattened
            glm::vec2 positions[4]; // The label positions if the label is flattened
        };

        glm::vec2 rotation = {1.f, 0.f};
        float alpha = 0.f;
    };

    struct Transition {
        FadeEffect::Interpolation ease = FadeEffect::Interpolation::linear;
        float time = 0.0;
    };

    struct Options {
        glm::vec2 offset;
        float priority = std::numeric_limits<float>::max();
        bool collide = true;
        Transition selectTransition;
        Transition hideTransition;
        Transition showTransition;
        size_t repeatGroup = 0;
        float repeatDistance = 0;
        float buffer = 0.f;
        size_t paramHash = 0; // the label hash based on its styling parameters
        LabelProperty::Anchors anchors;
        bool required = true;
        bool flat = false;
        float angle = 0.f;
        uint32_t featureId = 0;
    };

    static const float activation_distance_threshold;

    Label(WorldTransform _transform, glm::vec2 _size, Type _type, Options _options);

    virtual ~Label();

    // Add vertices for this label to its Style's shared Mesh
    virtual void addVerticesToMesh() = 0;
    virtual glm::vec2 center() const;
    virtual void updateBBoxes(float _zoomFract) = 0;

    virtual LabelType renderType() const = 0;

    // Update the screen position of the label
    virtual bool updateScreenTransform(const glm::mat4& _mvp, const ViewState& _viewState, bool _drawAllLabels) = 0;

    virtual uint32_t selectionColor() = 0;

    bool update(const glm::mat4& _mvp,
                const ViewState& _viewState,
                bool _drawAllLabels = false);

    bool evalState(float _dt);

    // Occlude the label
    void occlude(bool _occlusion = true) { m_occluded = _occlusion; }

    // Checks whether the label is in a state where it can occlusion
    bool canOcclude();

    void skipTransitions();

    size_t hash() const { return m_options.paramHash; }

    const glm::vec2& dimension() const { return m_dim; }

    // Gets for label options: color and offset
    const Options& options() const { return m_options; }

    // Gets the extent of the oriented bounding box of the label
    AABB aabb() const { return m_obb.getExtent(); }

    // Gets the oriented bounding box of the label
    const OBB& obb() const { return m_obb; }

    // The label world transform (position with the tile, in tile units)
    const WorldTransform& worldTransform() const { return m_worldTransform; }

    // The label screen transform, in a top left coordinate axis, y pointing down
    const ScreenTransform& screenTransform() const { return m_screenTransform; }

    State state() const { return m_state; }

    bool isOccluded() const { return m_occluded; }

    bool occludedLastFrame() const { return m_occludedLastFrame; }

    Label* parent() const { return m_parent; }
    void setParent(Label& parent, bool definePriority, bool defineCollide);

    LabelProperty::Anchor anchorType() const { return m_options.anchors[m_anchorIndex]; }

    int anchorIndex() { return m_anchorIndex; }

    bool nextAnchor();

    bool setAnchorIndex(int _index);

    // Returns the list of lon/lat of the feature the label is associated with
    LngLat coordinates(const Tile& _tile, const MapProjection& _projection);

    // Returns the screen distance squared from a screen coordinate
    float screenDistance2(glm::vec2 _screenPosition) const;

    // Returns the length of the segment the label is associated with
    float worldLineLength2() const;

    void enterState(const State& _state, float _alpha = 1.0f);

    void resetState();

    // Checks whether the label is in a visible state
    bool visibleState() const;

    Type type() const { return m_type; }

    void print() const;

    bool offViewport(const glm::vec2& _screenSize);

    void setAlpha(float _alpha);

private:

    virtual void applyAnchor(LabelProperty::Anchor _anchor) = 0;

    State m_state;

    FadeEffect m_fade;

    int m_anchorIndex;

protected:

    bool m_occludedLastFrame;

    bool m_occluded;

    Type m_type;

    OBB m_obb;

    WorldTransform m_worldTransform;

    ScreenTransform m_screenTransform;

    glm::vec2 m_dim;

    Options m_options;

    glm::vec2 m_anchor;

    Label* m_parent;

};

}

namespace std {
    template <>
    struct hash<Tangram::Label::Options> {
        size_t operator() (const Tangram::Label::Options& o) const {
            std::size_t seed = 0;
            hash_combine(seed, o.offset.x);
            hash_combine(seed, o.offset.y);
            hash_combine(seed, o.priority);
            hash_combine(seed, o.collide);
            hash_combine(seed, o.repeatDistance);
            hash_combine(seed, o.repeatGroup);
            hash_combine(seed, (int)o.selectTransition.ease);
            hash_combine(seed, o.selectTransition.time);
            hash_combine(seed, (int)o.hideTransition.ease);
            hash_combine(seed, o.hideTransition.time);
            hash_combine(seed, (int)o.showTransition.ease);
            hash_combine(seed, o.showTransition.time);
            for (int i = 0; i < o.anchors.count; ++i) {
                hash_combine(seed, (int)o.anchors[i]);
            }
            return seed;
        }
    };
}
