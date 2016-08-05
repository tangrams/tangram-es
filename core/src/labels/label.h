#pragma once

#include "glm/vec2.hpp"
#include "glm/mat4x4.hpp"
#include <climits> // needed in aabb.h
#include "aabb.h"
#include "obb.h"
#include "fadeEffect.h"
#include "util/types.h"
#include "util/hash.h"
#include "data/properties.h"
#include "labels/labelProperty.h"

#include <string>
#include <limits>
#include <memory>

namespace Tangram {

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
        anchor_fallback = 1 << 6,
        skip_transition = 1 << 7,
        dead            = 1 << 8,
    };

    enum class EvalUpdate {
        none,
        animate,
        relayout,
    };

    struct Transform {
        Transform(glm::vec2 _pos) : modelPosition1(_pos), modelPosition2(_pos) {}
        Transform(glm::vec2 _pos1, glm::vec2 _pos2) : modelPosition1(_pos1), modelPosition2(_pos2) {}

        glm::vec2 modelPosition1;
        glm::vec2 modelPosition2;

        struct {
            glm::vec2 screenPos;
            glm::vec2 rotation;
            float alpha = 0.f;
        } state;
    };

    struct Transition {
        FadeEffect::Interpolation ease = FadeEffect::Interpolation::sine;
        float time = 0.2;
    };

    struct Options {
        glm::vec2 offset;
        float priority = std::numeric_limits<float>::max();
        bool interactive = false;
        std::shared_ptr<Properties> properties;
        bool collide = true;
        Transition selectTransition;
        Transition hideTransition;
        Transition showTransition;
        size_t repeatGroup = 0;
        float repeatDistance = 0;
        float buffer = 0.f;

        // the label hash based on its styling parameters
        size_t paramHash = 0;

        LabelProperty::Anchors anchors;
    };

    static const float activation_distance_threshold;

    Label(Transform _transform, glm::vec2 _size, Type _type, Options _options);

    virtual ~Label();

    bool update(const glm::mat4& _mvp, const glm::vec2& _screenSize, float _zoomFract, bool _drawAllLabels = false);

    /* Push the pending transforms to the vbo by updating the vertices */
    virtual void pushTransform() = 0;

    EvalUpdate evalState(float _dt);

    /* Update the screen position of the label */
    bool updateScreenTransform(const glm::mat4& _mvp, const glm::vec2& _screenSize, bool _drawAllLabels);

    virtual void updateBBoxes(float _zoomFract) = 0;

    /* Occlude the label */
    void occlude(bool _occlusion = true) { m_occluded = _occlusion; }

    /* Checks whether the label is in a state where it can occlusion */
    bool canOcclude();

    void skipTransitions();

    /* Checks whether the label is in a visible state */
    bool visibleState() const;

    void resetState();

    size_t hash() const { return m_options.paramHash; }
    const glm::vec2& dimension() const { return m_dim; }
    /* Gets for label options: color and offset */
    const Options& options() const { return m_options; }
    /* Gets the extent of the oriented bounding box of the label */
    AABB aabb() const { return m_obb.getExtent(); }
    /* Gets the oriented bounding box of the label */
    const OBB& obb() const { return m_obb; }
    const Transform& transform() const { return m_transform; }
    State state() const { return m_state; }
    bool isOccluded() const { return m_occluded; }
    bool occludedLastFrame() const { return m_occludedLastFrame; }

    const Label* parent() const { return m_parent; }
    void setParent(const Label& _parent, bool _definePriority);

    void alignFromParent(const Label& _parent);

    LabelProperty::Anchor anchorType() const { return m_options.anchors[m_anchorIndex]; }

    virtual glm::vec2 center() const;

    void enterState(const State& _state, float _alpha = 1.0f);

    Type type() const { return m_type; }

    void print() const;

private:

    virtual void applyAnchor(const glm::vec2& _dimension, const glm::vec2& _origin,
                             LabelProperty::Anchor _anchor) = 0;

    bool offViewport(const glm::vec2& _screenSize);

    void setAlpha(float _alpha);

    // the current label state
    State m_state;
    // the label fade effect
    FadeEffect m_fade;

    int m_anchorIndex;

protected:

    // whether the label was occluded on the previous frame
    bool m_occludedLastFrame;
    bool m_occluded;

    // the label type (point/line)
    Type m_type;
    // the label oriented bounding box
    OBB m_obb;
    // the label transforms
    Transform m_transform;
    // the dimension of the label
    glm::vec2 m_dim;
    // label options
    Options m_options;

    glm::vec2 m_anchor;

    const Label* m_parent;

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
            hash_combine(seed, o.interactive);
            hash_combine(seed, o.collide);
            hash_combine(seed, o.repeatDistance);
            hash_combine(seed, o.repeatGroup);
            hash_combine(seed, (int)o.selectTransition.ease);
            hash_combine(seed, o.selectTransition.time);
            hash_combine(seed, (int)o.hideTransition.ease);
            hash_combine(seed, o.hideTransition.time);
            hash_combine(seed, (int)o.showTransition.ease);
            hash_combine(seed, o.showTransition.time);
            return seed;
        }
    };
}
