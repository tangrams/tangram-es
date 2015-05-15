#pragma once

#include "glm/glm.hpp"
#include "text/fontContext.h"
#include "text/textBuffer.h"
#include "isect2d.h"
#include <string>



class Label {

public:

    enum class Type {
        POINT,
        LINE,
        DEBUG
    };

    enum State {
        FADING_IN       = 1,
        FADING_OUT      = 1 << 1,
        VISIBLE         = 1 << 2,
        SLEEP           = 1 << 3,
        OUT_OF_SCREEN   = 1 << 4,
        WAIT_OCC        = 1 << 5, // state waiting for first occlusion result
        STATE_N         = 7
    };

    struct Transform {
        glm::vec2 m_modelPosition1;
        glm::vec2 m_modelPosition2;

        glm::vec2 m_screenPosition;

        float m_alpha;
        float m_rotation;
    };

    Label(Transform _transform, std::string _text, std::shared_ptr<TextBuffer> _buffer, Type _type);
    ~Label();

    /* Call the font context to rasterize the label string */
    void rasterize();

    Transform getTransform() const { return m_transform; }

    /* Update the transform of the label in world space, and project it to screen space */
    void updateTransform(const Transform& _transform, const glm::mat4& _mvp, const glm::vec2& _screenSize);

    /* gets the oriented bounding box of the label */
    const isect2d::OBB& getOBB() const { return m_obb; }

    /* gets the extent of the oriented bounding box of the label */
    const isect2d::AABB& getAABB() const { return m_aabb; }

    std::string getText() { return m_text; }

    void update(const glm::mat4& _mvp, const glm::vec2& _screenSize, float _dt);

    void pushTransform();

    bool updateScreenTransform(const glm::mat4& _mvp, const glm::vec2& _screenSize);

    Type getType() const { return m_type; }

    void setOcclusion(bool _occlusion);

    bool canOcclude();

    void resetOcclusion();

    void occlusionSolved();

    bool occludedLastFrame() { return m_occludedLastFrame; }

private:

    void enterState(State _state);

    bool offViewport(const glm::vec2& _screenSize);

    void updateBBoxes();

    void updateState(const glm::mat4& _mvp, const glm::vec2& _screenSize, float _dt);

    State m_currentState;

    Type m_type;
    Transform m_transform;
    std::string m_text;
    std::shared_ptr<TextBuffer> m_buffer; // the buffer in which this label text id is associated to
    fsuint m_id;
    isect2d::OBB m_obb;
    isect2d::AABB m_aabb;
    glm::vec2 m_dim;
    bool m_occludedLastFrame;
    bool m_occlusionSolved;
    float m_depth;

};
