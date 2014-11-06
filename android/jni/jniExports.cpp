
#include <jni.h>
#include "tangram.h"

extern "C" {
	JNIEXPORT void JNICALL Java_com_mapzen_tangram_TangramRenderer_init(JNIEnv* jniEnv, jobject obj) 
	{
		Tangram::initialize();
	}

	JNIEXPORT void JNICALL Java_com_mapzen_tangram_TangramRenderer_resize(JNIEnv* jniEnv, jobject obj, jint width, jint height) 
	{
		Tangram::resize(width, height);
	}

	JNIEXPORT void JNICALL Java_com_mapzen_tangram_TangramRenderer_update(JNIEnv* jniEnv, jobject obj, jfloat dt)
	{
		Tangram::update(dt);
	}

	JNIEXPORT void JNICALL Java_com_mapzen_tangram_TangramRenderer_render(JNIEnv* jniEnv, jobject obj) 
	{
		Tangram::render();
	}
}


