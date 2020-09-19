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

void JniHelpers::jniOnLoad(JavaVM* jvm, JNIEnv* jniEnv) {
    s_jvm = jvm;

    jclass cameraPositionClass = jniEnv->FindClass("com/mapzen/tangram/CameraPosition");
    cameraPositionLongitudeFID = jniEnv->GetFieldID(cameraPositionClass, "longitude", "D");
    cameraPositionLatitudeFID = jniEnv->GetFieldID(cameraPositionClass, "latitude", "D");
    cameraPositionZoomFID = jniEnv->GetFieldID(cameraPositionClass, "zoom", "F");
    cameraPositionRotationFID = jniEnv->GetFieldID(cameraPositionClass, "rotation", "F");
    cameraPositionTiltFID = jniEnv->GetFieldID(cameraPositionClass, "tilt", "F");
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

} // namespace Tangram
