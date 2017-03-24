#include "fixfloat.h"

#include "double-conversion.h"

#include <locale>
#include <iomanip>
#include <sstream>

namespace Tangram {


using namespace double_conversion;

StringToDoubleConverter S2D = {
    StringToDoubleConverter::ALLOW_TRAILING_JUNK |
    StringToDoubleConverter::ALLOW_LEADING_SPACES,
    0.0, NAN, "inf", "nan" };

class DecimalPoint: public std::numpunct<char> {
protected: char do_decimal_point() const { return '.'; }
};

std::locale C_LOCALE(std::locale(), new DecimalPoint);

double ff::stod(const char* _string, int _length, int* _end) {
    return S2D.StringToDouble(_string, _length, _end);
}

float ff::stof(const char* _string, int _length, int* _end) {
    return S2D.StringToFloat(_string, _length, _end);
}

std::string ff::to_string(glm::vec2 _vec) {
        std::stringstream out;
        out.imbue(C_LOCALE);
        out << std::fixed;

        out << "vec2(" << _vec[0] << "," << _vec[1] << ")";
        return out.str();
    }

 std::string ff::to_string(glm::vec3 _vec) {
        std::stringstream out;
        out.imbue(C_LOCALE);
        out << std::fixed;

        out << "vec3(" << _vec[0] << "," << _vec[1] << "," << _vec[2] << ")";
        return out.str();
    }

 std::string ff::to_string(glm::vec4 _vec) {
        std::stringstream out;
        out.imbue(C_LOCALE);
        out << std::fixed;

        out << "vec4(" << _vec[0] << "," << _vec[1] << "," << _vec[2] << "," << _vec[3] << ")";
        return out.str();
    }

 std::string ff::to_string(float _value) {
        std::stringstream out;
        out.imbue(C_LOCALE);
        out << std::fixed;

        out << _value;
        return out.str();
    }

}
