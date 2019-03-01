//
// Created by Matt Blair on 5/21/18.
//
#pragma once
#include "annotation/annotationRenderer.h"
#include <jni.h>

namespace Tangram {

class AndroidCustomRenderer : public CustomRenderer {

public:

    AndroidCustomRenderer(JNIEnv* jniEnv, jobject jrenderer);

    void dispose(JNIEnv* jniEnv);

    jobject renderer() { return m_jrenderer; }

    // Called when the renderer is added to the map.
    void initialize() override;

    // Called on the GL thread during each rendered frame.
    void render(const CustomRenderContext& context) override;

    // Called when the map is destroyed or the renderer is removed.
    void deinitialize() override;

private:

    jobject m_jrenderer;

    static void globalSetup(JNIEnv* jniEnv);

    static jclass customRendererClass;
    static jmethodID customRendererInitializeMID;
    static jmethodID customRendererRenderMID;
    static jmethodID customRendererDeinitializeMID;
};

} // namespace Tangram
