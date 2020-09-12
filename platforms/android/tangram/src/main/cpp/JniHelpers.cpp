#include "JniHelpers.h"
#include <codecvt>
#include <locale>

namespace Tangram {

JavaVM* JniHelpers::s_jvm = nullptr;

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

} // namespace Tangram
