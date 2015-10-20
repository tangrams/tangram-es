#pragma once

#include "platform.h"
#include "data/properties.h"

#include <memory>
#include <jni.h>

void setupJniEnv(JNIEnv* _jniEnv, jobject _tangramInstance, jobject _assetManager);
void onUrlSuccess(JNIEnv* jniEnv, jbyteArray jFetchedBytes, jlong jCallbackPtr);
void onUrlFailure(JNIEnv* jniEnv, jlong jCallbackPtr);

namespace Tangram {
struct TouchItem;
}

void featureSelectionCallback(JNIEnv* jniEnv, const std::vector<Tangram::TouchItem>& items);
