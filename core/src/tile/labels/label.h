#pragma once

#include "glm/glm.hpp"
#include "text/textBuffer.h"
#include "isect2d.h"
#include <string>

struct FadeEffect {

public:
    enum class Interpolation { linear, pow, sine };

    FadeEffect() {}

    FadeEffect(bool _in, Interpolation _interpolation, float _duration)
        : m_interpolation(_interpolation), m_duration(_duration), m_in(_in) {}

    float update(float _dt) {
        m_step += _dt;
        float st = m_step / m_duration;

        switch (m_interpolation) {
        case Interpolation::linear: return m_in ? st : -st + 1;
        case Interpolation::pow: return m_in ? st * st : -(st * st) + 1;
        case Interpolation::sine: return m_in ? sin(st * M_PI * 0.5) : cos(st * M_PI * 0.5);
        }

        return st;
    }

    bool isFinished() { return m_step > m_duration; }

private:
    Interpolation m_interpolation = Interpolation::linear;
    float m_duration = 0.5;
    float m_step = 0.0;
    bool m_in;
};

class Label {

public:
    enum class Type { point, line, debug };

    enum State {
        fading_in = 1,
        fading_out = 1 << 1,
        visible = 1 << 2,
        sleep = 1 << 3,
        out_of_screen = 1 << 4,
        wait_occ = 1 << 5, // state waiting for first occlusion result
    };

    struct Transform {
        glm::vec2 m_modelPosition1;
        glm::vec2 m_modelPosition2;

        glm::vec2 m_screenPosition;

        float m_alpha;
        float m_rotation;
    };

    Label(Transform _transform, std::string _text, fsuint _id, Type _type);

    ~Label();

    /* Call the font context to rasterize the label string */
    bool rasterize(std::shared_ptr<TextBuffer>& _buffer);

    Transform getTransform() const { return m_transform; }

    /* Update the transform of the label in world space, and project it to screen space */
    void updateTransform(const Transform& _transform, const glm::mat4& _mvp, const glm::vec2& _screenSize);

    /* gets the oriented bounding box of the label */
    const isect2d::OBB& getOBB() const { return m_obb; }

    /* gets the extent of the oriented bounding box of the label */
    const isect2d::AABB& getAABB() const { return m_aabb; }

    std::string getText() { return m_text; }

    void update(const glm::mat4& _mvp, const glm::vec2& _screenSize, float _dt);

    void pushTransform(std::shared_ptr<TextBuffer>& _buffer);

    bool updateScreenTransform(const glm::mat4& _mvp, const glm::vec2& _screenSize);

    Type getType() const { return m_type; }

    void setOcclusion(bool _occlusion);

    bool canOcclude();

    bool offViewport(const glm::vec2& _screenSize);

    void occlusionSolved();

    bool occludedLastFrame() { return m_occludedLastFrame; }

    State getState() const { return m_currentState; }

    static bool s_needUpdate;

private:
    void enterState(State _state, float _alpha = 1.0f);

    void updateBBoxes();

    void updateState(const glm::mat4& _mvp, const glm::vec2& _screenSize, float _dt);

    void setAlpha(float _alpha);

    void setScreenPosition(const glm::vec2& _screenPosition);

    void setRotation(float _rotation);

    State m_currentState;

    Type m_type;
    Transform m_transform;
    std::string m_text;
    fsuint m_id;
    isect2d::OBB m_obb;
    isect2d::AABB m_aabb;
    glm::vec2 m_dim;
    bool m_occludedLastFrame;
    bool m_occlusionSolved;
    FadeEffect m_fade;
    bool m_dirty;
};
