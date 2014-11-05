
#include <jni.h>
#include "tangram.h"

extern "C" {
	JNIEXPORT void JNICALL Java_com_mapzen_tangram_TangramRenderer_init(JNIEnv* jniEnv, jobject obj, jobject assetManager) 
	{
		initializeOpenGL();
		setAssetManager(jniEnv, assetManager);
	}

	JNIEXPORT void JNICALL Java_com_mapzen_tangram_TangramRenderer_resize(JNIEnv* jniEnv, jobject obj, jint width, jint height) 
	{
		resizeViewport(width, height);
	}

	JNIEXPORT void JNICALL Java_com_mapzen_tangram_TangramRenderer_render(JNIEnv* jniEnv, jobject obj) 
	{
		renderFrame();
	}
}


