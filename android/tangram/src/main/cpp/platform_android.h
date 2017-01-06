#pragma once

#include "platform.h"

#include <memory>
#include <jni.h>

void bindJniEnvToThread(JNIEnv* jniEnv);
void setupJniEnv(JNIEnv* _jniEnv, jobject _tangramInstance, jobject _assetManager);
void onUrlSuccess(JNIEnv* jniEnv, jbyteArray jFetchedBytes, jlong jCallbackPtr);
void onUrlFailure(JNIEnv* jniEnv, jlong jCallbackPtr);

namespace Tangram {
struct LabelPickResult;
struct FeaturePickResult;
struct MarkerPickResult;
}

void featurePickCallback(jobject listener, const Tangram::FeaturePickResult* featurePickResult);
void markerPickCallback(jobject listener, const Tangram::MarkerPickResult* markerPickResult);
void labelPickCallback(jobject listener, const Tangram::LabelPickResult* labelPickResult);

std::string resolveScenePath(const char* path);

std::string stringFromJString(JNIEnv* jniEnv, jstring string);

