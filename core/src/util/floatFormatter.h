#pragma once
#include <string>
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

namespace Tangram {

struct ff {

    static std::string to_string(glm::vec2 _vec);

    static std::string to_string(glm::vec3 _vec);

    static std::string to_string(glm::vec4 _vec);

    static std::string to_string(float _value);

    static double stod(const char* _string, int _length, int* _end);

    static double stod(const std::string& _string) {
        int end = 0;
        return stod(_string.data(), _string.size(), &end);
    }

    static float stof(const char* _string, int _length, int* _end);

    static float stof(const std::string& _string) {
        int end = 0;
        return stof(_string.data(), _string.size(), &end);
    }

};

}
