#pragma once

#include "labels/fadeEffect.h"
#include "labels/labelProperty.h"
#include "tangram.h"
#include "util/types.h"
#include "util/hash.h"

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include <climits> // needed in aabb.h
#include "aabb.h"
#include "obb.h"
#include <string>
#include <limits>
#include <memory>

namespace Tangram {

struct ScreenTransform;
struct ViewState;
struct OBBBuffer;

class Label {

public:

    using OBB = isect2d::OBB<glm::vec2>;
    using AABB = isect2d::AABB<glm::vec2>;

    enum class Type {
        point,
        line,
        curved,
        debug
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

    struct Transition {
        FadeEffect::Interpolation ease = FadeEffect::Interpolation::linear;
        float time = 0.0;
    };

    struct Options {
        glm::vec2 offset;
        glm::vec2 buffer;
        float priority = std::numeric_limits<float>::max();
        bool collide = true;
        Transition selectTransition;
        Transition hideTransition;
        Transition showTransition;
        size_t repeatGroup = 0;
        float repeatDistance = 0;
        size_t paramHash = 0; // the label hash based on its styling parameters
        LabelProperty::Anchors anchors;
        bool optional = false;
        bool flat = false;
        float angle = 0.f;
        uint32_t featureId = 0;
    };

    static const float activation_distance_threshold;

    Label(glm::vec2 _size, Type _type, Options _options);

    virtual ~Label();

    // Add vertices for this label to its Style's shared Mesh
    virtual void addVerticesToMesh(ScreenTransform& _transform, const glm::vec2& _screenSize) = 0;

    virtual LabelType renderType() const = 0;

    virtual uint32_t selectionColor() = 0;

    virtual glm::vec2 modelCenter() const = 0;

    // Returns the candidate priority for a feature with multiple labels
    virtual float candidatePriority() const { return 0; }

    bool update(const glm::mat4& _mvp, const ViewState& _viewState,
                const AABB* _bounds, ScreenTransform& _transform);

    bool evalState(float _dt);

    // Update the screen position of the label
    virtual bool updateScreenTransform(const glm::mat4& _mvp, const ViewState& _viewState,
                                       const AABB* _bounds, ScreenTransform& _transform) = 0;

    // Current screen position of the label anchor
    glm::vec2 screenCenter() const { return m_screenCenter; }

    // Occlude the label
    void occlude(bool _occlusion = true) { m_occluded = _occlusion; }

    // Checks whether the label is in a state where it can occlusion
    bool canOcclude() const { return m_options.collide; }

    void skipTransitions();

    size_t hash() const { return m_options.paramHash; }

    glm::vec2 dimension() const { return m_dim; }

    // Gets for label options: color and offset
    const Options& options() const { return m_options; }

    // Adds the oriented bounding boxes of the label to _obbs, updates Range
    virtual void obbs(ScreenTransform& _transform, OBBBuffer& _obbs) = 0;

    State state() const { return m_state; }

    bool isOccluded() const { return m_occluded; }

    bool occludedLastFrame() const { return m_occludedLastFrame; }

    Label* parent() const { return m_parent; }
    void setParent(Label& parent, bool definePriority, bool defineCollide);

    LabelProperty::Anchor anchorType() const {
        return m_options.anchors[m_anchorIndex];
    }

    int anchorIndex() { return m_anchorIndex; }

    bool nextAnchor();

    bool setAnchorIndex(int _index);

    void enterState(const State& _state, float _alpha = 1.0f);

    void resetState();

    // Checks whether the label is in a visible state
    bool visibleState() const;

    Type type() const { return m_type; }

    void print() const;

    void setAlpha(float _alpha);

protected:

    virtual void applyAnchor(LabelProperty::Anchor _anchor) = 0;

    const Type m_type;
    const glm::vec2 m_dim;

    Options m_options;

    Label* m_parent;

    State m_state;
    FadeEffect m_fade;

    int m_anchorIndex;
    glm::vec2 m_anchor;

    bool m_occludedLastFrame;
    bool m_occluded;

    glm::vec2 m_screenCenter;
    float m_alpha;
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
