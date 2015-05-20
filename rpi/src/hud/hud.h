#pragma once

#include "util/shaderProgram.h"

#include "shapes.h"
#include "glm/glm.hpp"

#include "slideZoom.h"
#include "slideRot.h"

class Hud {
public:

    Hud();
    virtual ~Hud();

    void setDrawCursor(bool _true);

    void init();
    void draw();

    void cursorClick(float _x, float _y, int _button);
    void cursorDrag(float _x, float _y, int _button);
    void cursorRelease(float _x, float _y);

    bool isInUse();

    SlideRot m_rot;
    SlideZoom m_zoom;

private:

    std::shared_ptr<ShaderProgram>  m_trnShader;
    std::shared_ptr<HudMesh>       m_cursorMesh;  

    int     m_selected; 
    bool    m_bCursor; 
};