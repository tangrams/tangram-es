#pragma once

#include "platform.h"

#include <memory>
#include <jni.h>

void bindJniEnvToThread(JNIEnv* jniEnv);
void setupJniEnv(JNIEnv* _jniEnv, jobject _tangramInstance, jobject _assetManager);
void onUrlSuccess(JNIEnv* jniEnv, jbyteArray jFetchedBytes, jlong jCallbackPtr);
void onUrlFailure(JNIEnv* jniEnv, jlong jCallbackPtr);

namespace Tangram {
struct TouchItem;
struct TouchLabel;
}

void featurePickCallback(jobject listener, const std::vector<Tangram::TouchItem>& items);

std::string resolveScenePath(const char* path);

void labelsPickCallback(jobject listener, const std::vector<Tangram::TouchLabel>& labels);

std::string stringFromJString(JNIEnv* jniEnv, jstring string);
