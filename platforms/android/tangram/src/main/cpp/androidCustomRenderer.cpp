//
// Created by Matt Blair on 5/21/18.
//

#include "androidCustomRenderer.h"
#include "androidPlatform.h"
#include "jniThreadBinding.h"

namespace Tangram {

jclass AndroidCustomRenderer::customRendererClass = nullptr;
jmethodID AndroidCustomRenderer::customRendererInitializeMID = 0;
jmethodID AndroidCustomRenderer::customRendererRenderMID = 0;
jmethodID AndroidCustomRenderer::customRendererDeinitializeMID = 0;

void AndroidCustomRenderer::globalSetup(JNIEnv* jniEnv) {
    customRendererClass = jniEnv->FindClass("com/mapzen/tangram/CustomRenderer");
    customRendererInitializeMID = jniEnv->GetMethodID(customRendererClass, "initialize", "()V");
    customRendererRenderMID = jniEnv->GetMethodID(customRendererClass, "render", "(DDDDDDDD)V");
    customRendererDeinitializeMID = jniEnv->GetMethodID(customRendererClass, "deinitialize", "()V");
}

AndroidCustomRenderer::AndroidCustomRenderer(JNIEnv* jniEnv, jobject jrenderer) {
    if (!customRendererClass) {
        globalSetup(jniEnv);
    }
    m_jrenderer = jniEnv->NewGlobalRef(jrenderer);
}

void AndroidCustomRenderer::dispose(JNIEnv* jniEnv) {
    jniEnv->DeleteGlobalRef(m_jrenderer);
}

void AndroidCustomRenderer::initialize() {
    JniThreadBinding jniEnv(AndroidPlatform::getJvm());
    jniEnv->CallVoidMethod(m_jrenderer, customRendererInitializeMID);
}

void AndroidCustomRenderer::render(const CustomRenderContext& context) {
    JniThreadBinding jniEnv(AndroidPlatform::getJvm());
    jniEnv->CallVoidMethod(m_jrenderer, customRendererRenderMID,
        context.width,
        context.height,
        context.longitude,
        context.latitude,
        context.zoom,
        context.rotation,
        context.tilt,
        context.fieldOfView
    );
}

void AndroidCustomRenderer::deinitialize() {
    JniThreadBinding jniEnv(AndroidPlatform::getJvm());
    jniEnv->CallVoidMethod(m_jrenderer, customRendererDeinitializeMID);
}

} // namespace Tangram
