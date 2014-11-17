
#include <jni.h>
#include "tangram.h"
// Includes platform.h for setAssetManager reference
#include "platform.h"

extern "C" {
	JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_init(JNIEnv* jniEnv, jobject obj, jobject assetManager) 
	{
		setAssetManager(jniEnv, assetManager);
		Tangram::initialize();
	}

	JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_resize(JNIEnv* jniEnv, jobject obj, jint width, jint height) 
	{
		Tangram::resize(width, height);
	}

	JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_update(JNIEnv* jniEnv, jobject obj, jfloat dt)
	{
		Tangram::update(dt);
	}

	JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_render(JNIEnv* jniEnv, jobject obj) 
	{
		Tangram::render();
	}

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_handleTapGestures(JNIEnv* jniEnv, jobject obj, jfloat posX, jfloat posY) {
        Tangram::handleTapGestures(glm::vec2(posX, posY));
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_handleDoubleTapGestures(JNIEnv* jniEnv, jobject obj, jfloat posX, jfloat posY) {
        Tangram::handleDoubleTapGestures(glm::vec2(posX, posY));
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_handlePanGestures(JNIEnv* jniEnv, jobject obj, jfloat velX, jfloat velY) {
        Tangram::handlePanGestures(glm::vec2(velX, velY));
    }

    JNIEXPORT void JNICALL Java_com_mapzen_tangram_Tangram_handlePinchGestures(JNIEnv* jniEnv, jobject obj, jfloat posX, jfloat posY, jfloat scale) {
        Tangram::handlePinchGestures(glm::vec2(posX, posY), scale);
    }
    
}

