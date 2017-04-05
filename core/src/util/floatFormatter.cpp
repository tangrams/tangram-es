#include "util/floatFormatter.h"

#include "double-conversion.h"
#include <cmath>

namespace Tangram {


using namespace double_conversion;

StringToDoubleConverter S2D = {
    StringToDoubleConverter::ALLOW_TRAILING_JUNK |
    StringToDoubleConverter::ALLOW_LEADING_SPACES,
    0.0, NAN, "inf", "nan" };

DoubleToStringConverter D2S = {
    DoubleToStringConverter::EMIT_TRAILING_DECIMAL_POINT |
    DoubleToStringConverter::EMIT_TRAILING_ZERO_AFTER_POINT |
    DoubleToStringConverter::UNIQUE_ZERO,
    "inf", "nan", 'e', -10, 10, 6, 6 };

const size_t StringBuilderBufferSize = 256;

double ff::stod(const char* _string, int _length, int* _end) {
    return S2D.StringToDouble(_string, _length, _end);
}

float ff::stof(const char* _string, int _length, int* _end) {
    return S2D.StringToFloat(_string, _length, _end);
}

std::string ff::to_string(glm::vec2 _vec) {
    char buffer[StringBuilderBufferSize];
    StringBuilder builder(buffer, StringBuilderBufferSize);
    builder.AddString("vec2(");
    D2S.ToShortest(_vec[0], &builder);
    builder.AddCharacter(',');
    D2S.ToShortest(_vec[1], &builder);
    builder.AddCharacter(')');
    return std::string(builder.Finalize());
}

std::string ff::to_string(glm::vec3 _vec) {
    char buffer[StringBuilderBufferSize];
    StringBuilder builder(buffer, StringBuilderBufferSize);
    builder.AddString("vec3(");
    D2S.ToShortest(_vec[0], &builder);
    builder.AddCharacter(',');
    D2S.ToShortest(_vec[1], &builder);
    builder.AddCharacter(',');
    D2S.ToShortest(_vec[2], &builder);
    builder.AddCharacter(')');
    return std::string(builder.Finalize());
}

std::string ff::to_string(glm::vec4 _vec) {
    char buffer[StringBuilderBufferSize];
    StringBuilder builder(buffer, StringBuilderBufferSize);
    builder.AddString("vec4(");
    D2S.ToShortest(_vec[0], &builder);
    builder.AddCharacter(',');
    D2S.ToShortest(_vec[1], &builder);
    builder.AddCharacter(',');
    D2S.ToShortest(_vec[2], &builder);
    builder.AddCharacter(',');
    D2S.ToShortest(_vec[3], &builder);
    builder.AddCharacter(')');
    return std::string(builder.Finalize());
}

std::string ff::to_string(float _value) {
    char buffer[StringBuilderBufferSize];
    StringBuilder builder(buffer, StringBuilderBufferSize);
    D2S.ToShortest(_value, &builder);
    return std::string(builder.Finalize());
}

}
