#pragma once

#include "glm/vec2.hpp"
#include "glm/mat4x4.hpp"
#include <climits> // needed in aabb.h
#include "isect2d.h"
#include "glm_vec.h" // for isect2d.h
#include "fadeEffect.h"
#include "util/types.h"
#include "util/hash.h"
#include "data/properties.h"

#include <string>
#include <limits>
#include <memory>

namespace {
// Conversion factors for Label::Vertex attributes
constexpr float position_scale = 4.0f;
constexpr float extrusion_scale = 256.0f;
constexpr float rotation_scale = 4096.0f;
constexpr float alpha_scale = 32767.f;
}

namespace Tangram {

class LabelMesh;

class Label {

public:

    using OBB = isect2d::OBB<glm::vec2>;
    using AABB = isect2d::AABB<glm::vec2>;

    enum class Type {
        point,
        line,
        debug
    };

    enum State {
        fading_in       = 1,
        fading_out      = 1 << 1,
        visible         = 1 << 2,
        sleep           = 1 << 3,
        out_of_screen   = 1 << 4,
        wait_occ        = 1 << 5, // state waiting for first occlusion result
        skip_transition = 1 << 6,
        dead            = 1 << 7,
    };

    struct Vertex {
        // Constructor for TextStyle vertices
        Vertex(glm::vec2 pos, glm::vec2 uv, uint32_t color, uint32_t stroke)
            : pos(glm::round(pos * position_scale)), uv(uv),
              color(color), stroke(stroke) {}

        // Constructor for PointStyle vertices
        Vertex(glm::vec2 pos, glm::vec2 uv, glm::vec2 extrude, uint32_t color)
            : pos(glm::round(pos * position_scale)), uv(uv),
              color(color),
              extrude(extrude * extrusion_scale) {}

        glm::i16vec2 pos;
        glm::u16vec2 uv;
        uint32_t color;
        union {
            glm::i16vec2 extrude;
            uint32_t stroke;
        };
        struct State {
            State() {}
            State(glm::vec2 pos, float alpha, float rotation)
                : screenPos(pos * position_scale),
                  alpha(alpha * alpha_scale),
                  rotation(rotation * rotation_scale) {}

            glm::i16vec2 screenPos;
            short alpha = 0;
            short rotation = 0;
        } state;
    };

    struct Transform {
        Transform(glm::vec2 _pos) : modelPosition1(_pos), modelPosition2(_pos) {}
        Transform(glm::vec2 _pos1, glm::vec2 _pos2) : modelPosition1(_pos1), modelPosition2(_pos2) {}

        glm::vec2 modelPosition1;
        glm::vec2 modelPosition2;

        struct {
            glm::vec2 screenPos;
            float alpha = 0.f;
            float rotation = 0.f;
            Vertex::State vertex() { return Vertex::State(screenPos, alpha, rotation); }
        } state;
    };

    struct Transition {
        FadeEffect::Interpolation ease = FadeEffect::Interpolation::sine;
        float time = 0.2;
    };

    struct Options {
        glm::vec2 offset;
        uint32_t priority = std::numeric_limits<uint32_t>::max();
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
    };

    Label(Transform _transform, glm::vec2 _size, Type _type, LabelMesh& _mesh, Range _vertexRange,
            Options _options);

    virtual ~Label();

    bool update(const glm::mat4& _mvp, const glm::vec2& _screenSize, float _zoomFract);

    /* Push the pending transforms to the vbo by updating the vertices */
    void pushTransform();

    bool evalState(const glm::vec2& _screenSize, float _dt);

    /* Update the screen position of the label */
    bool updateScreenTransform(const glm::mat4& _mvp, const glm::vec2& _screenSize,
                               bool _testVisibility = true);

    virtual void updateBBoxes(float _zoomFract) = 0;

    /* Occlude the label */
    void occlude(bool _occlusion = true);

    /* Checks whether the label is in a state where it can occlusion */
    bool canOcclude();

    void skipTransitions();

    /* Checks whether the label is in a visible state */
    bool visibleState() const;

    void resetState();

    void setProxy(bool _proxy);

    /* Whether the label belongs to a proxy tile */
    bool isProxy() const { return m_proxy; }
    size_t hash() const { return m_options.paramHash; }
    const glm::vec2& dimension() const { return m_dim; }
    /* Gets for label options: color and offset */
    const Options& options() const { return m_options; }
    /* Gets the extent of the oriented bounding box of the label */
    const AABB& aabb() const { return m_aabb; }
    /* Gets the oriented bounding box of the label */
    const OBB& obb() const { return m_obb; }
    const Transform& transform() const { return m_transform; }
    State state() const { return m_state; }
    bool isOccluded() const { return m_occluded; }
    bool occludedLastFrame() const { return m_occludedLastFrame; }

    virtual glm::vec2 center() const;

private:

    bool offViewport(const glm::vec2& _screenSize);

    inline void enterState(const State& _state, float _alpha = 1.0f);

    void setAlpha(float _alpha);

    bool m_proxy;
    // the current label state
    State m_state;
    // the label fade effect
    FadeEffect m_fade;
    // whether the label was occluded on the previous frame
    bool m_occludedLastFrame;
    bool m_occluded;
    // whether or not we need to update the mesh visibilit (alpha channel)
    bool m_updateMeshVisibility;

protected:

    // set alignment on _screenPosition based on anchor points _ap1, _ap2
    virtual void align(glm::vec2& _screenPosition, const glm::vec2& _ap1, const glm::vec2& _ap2) = 0;

    // the label type (point/line)
    Type m_type;
    // the label oriented bounding box
    OBB m_obb;
    // the label axis aligned bounding box
    AABB m_aabb;
    // whether the label is dirty, this determines whether or no to update the geometry
    bool m_dirty;
    // the label transforms
    Transform m_transform;
    // the dimension of the label
    glm::vec2 m_dim;
    // Back-pointer to owning container
    LabelMesh& m_mesh;
    // first vertex and count in m_mesh vertices
    Range m_vertexRange;
    // label options
    Options m_options;

    glm::vec2 m_xAxis;
    glm::vec2 m_yAxis;
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

