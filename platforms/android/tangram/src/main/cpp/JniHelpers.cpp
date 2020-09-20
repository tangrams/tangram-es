#include "JniHelpers.h"
#include "map.h"
#include <codecvt>
#include <locale>

namespace Tangram {

JavaVM* JniHelpers::s_jvm = nullptr;

static jfieldID cameraPositionLongitudeFID = nullptr;
static jfieldID cameraPositionLatitudeFID = nullptr;
static jfieldID cameraPositionZoomFID = nullptr;
static jfieldID cameraPositionRotationFID = nullptr;
static jfieldID cameraPositionTiltFID = nullptr;

static jfieldID lngLatLongitudeFID = nullptr;
static jfieldID lngLatLatitudeFID = nullptr;

static jfieldID pointFxFID = nullptr;
static jfieldID pointFyFID = nullptr;

static jfieldID rectLeftFID = nullptr;
static jfieldID rectTopFID = nullptr;
static jfieldID rectRightFID = nullptr;
static jfieldID rectBottomFID = nullptr;

void JniHelpers::jniOnLoad(JavaVM* jvm, JNIEnv* jniEnv) {
    s_jvm = jvm;

    jclass cameraPositionClass = jniEnv->FindClass("com/mapzen/tangram/CameraPosition");
    cameraPositionLongitudeFID = jniEnv->GetFieldID(cameraPositionClass, "longitude", "D");
    cameraPositionLatitudeFID = jniEnv->GetFieldID(cameraPositionClass, "latitude", "D");
    cameraPositionZoomFID = jniEnv->GetFieldID(cameraPositionClass, "zoom", "F");
    cameraPositionRotationFID = jniEnv->GetFieldID(cameraPositionClass, "rotation", "F");
    cameraPositionTiltFID = jniEnv->GetFieldID(cameraPositionClass, "tilt", "F");

    jclass lngLatClass = jniEnv->FindClass("com/mapzen/tangram/LngLat");
    lngLatLongitudeFID = jniEnv->GetFieldID(lngLatClass, "longitude", "D");
    lngLatLatitudeFID = jniEnv->GetFieldID(lngLatClass, "latitude", "D");

    jclass pointFClass = jniEnv->FindClass("android/graphics/PointF");
    pointFxFID = jniEnv->GetFieldID(pointFClass, "x", "F");
    pointFyFID = jniEnv->GetFieldID(pointFClass, "y", "F");

    jclass rectClass = jniEnv->FindClass("android/graphics/Rect");
    rectLeftFID = jniEnv->GetFieldID(rectClass, "left", "I");
    rectTopFID = jniEnv->GetFieldID(rectClass, "top", "I");
    rectRightFID = jniEnv->GetFieldID(rectClass, "right", "I");
    rectBottomFID = jniEnv->GetFieldID(rectClass, "bottom", "I");
}

std::string JniHelpers::stringFromJavaString(JNIEnv* jniEnv, jstring javaString) {
    auto length = jniEnv->GetStringLength(javaString);
    std::u16string chars(length, char16_t());
    if(!chars.empty()) {
        jniEnv->GetStringRegion(javaString, 0, length, reinterpret_cast<jchar*>(&chars[0]));
    }
    return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>().to_bytes(chars);
}

jstring JniHelpers::javaStringFromString(JNIEnv* jniEnv, const std::string& string) {
    auto chars = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>().from_bytes(string);
    auto s = reinterpret_cast<const jchar*>(chars.empty() ? u"" : chars.data());
    return jniEnv->NewString(s, chars.length());
}

void JniHelpers::cameraPositionToJava(JNIEnv* env, jobject javaCamera, const CameraPosition& camera) {
    env->SetDoubleField(javaCamera, cameraPositionLongitudeFID, camera.longitude);
    env->SetDoubleField(javaCamera, cameraPositionLatitudeFID, camera.latitude);
    env->SetFloatField(javaCamera, cameraPositionZoomFID, camera.zoom);
    env->SetFloatField(javaCamera, cameraPositionRotationFID, camera.rotation);
    env->SetFloatField(javaCamera, cameraPositionTiltFID, camera.tilt);
}

void JniHelpers::vec2ToJava(JNIEnv* env, jobject javaPointF, float x, float y) {
    env->SetFloatField(javaPointF, pointFxFID, x);
    env->SetFloatField(javaPointF, pointFyFID, y);
}

void JniHelpers::lngLatToJava(JNIEnv* env, jobject javaLngLat, const LngLat& lngLat) {
    env->SetDoubleField(javaLngLat, lngLatLongitudeFID, lngLat.longitude);
    env->SetDoubleField(javaLngLat, lngLatLatitudeFID, lngLat.latitude);
}

LngLat JniHelpers::lngLatFromJava(JNIEnv* env, jobject javaLngLat) {
    double longitude = env->GetDoubleField(javaLngLat, lngLatLongitudeFID);
    double latitude = env->GetDoubleField(javaLngLat, lngLatLatitudeFID);
    return LngLat{ longitude, latitude };
}

EdgePadding JniHelpers::edgePaddingFromJava(JNIEnv* env, jobject javaRect) {
    int left = env->GetIntField(javaRect, rectLeftFID);
    int top = env->GetIntField(javaRect, rectTopFID);
    int right = env->GetIntField(javaRect, rectRightFID);
    int bottom = env->GetIntField(javaRect, rectBottomFID);
    return EdgePadding{ left, top, right, bottom };
}

} // namespace Tangram
