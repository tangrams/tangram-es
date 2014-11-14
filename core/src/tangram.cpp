#include "tangram.h"

// SceneDirector is the primary controller of the map
std::unique_ptr<SceneDirector> sceneDirector;

void initializeOpenGL()
{

    sceneDirector.reset(new SceneDirector());
    sceneDirector->loadStyles();

    logMsg("%s\n", "initialize");

}

void resizeViewport(int newWidth, int newHeight)
{
    glViewport(0, 0, newWidth, newHeight);

    if (sceneDirector) {
        sceneDirector->onResize(newWidth, newHeight);
    }

    logMsg("%s\n", "resizeViewport");
}

void renderFrame()
{

    // TODO: Use dt from client application
    sceneDirector->update(0.016);

    sceneDirector->renderFrame();

    GLenum glError = glGetError();
    if (glError) {
        logMsg("GL Error %d!!!\n", glError);
    }

}

void handleGestures(Tangram::Gestures _gesture, const glm::vec2 _posOrVel1, const float _scale) {
    switch(_gesture) {
        case Tangram::Gestures::Tap:
            sceneDirector->onTap(_posOrVel1);
            break;
        case Tangram::Gestures::DoubleTap:
            sceneDirector->onDoubleTap(_posOrVel1);
            break;
        case Tangram::Gestures::Pan:
            sceneDirector->onPan(_posOrVel1);
            break;
        case Tangram::Gestures::Pinch:
            sceneDirector->onPinch(_posOrVel1, _scale);
            break;
        case Tangram::Gestures::Invalid:
        default:
            logMsg("Gesture state invalid.\n");
            break;
    }

}

