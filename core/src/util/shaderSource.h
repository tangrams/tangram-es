#pragma once

namespace Tangram {

struct ShaderSource {
    std::string string;
    ShaderSource& operator<<(const std::string _str) {
        string += _str;
        string += '\n';
        return *this;
    }
    ShaderSource& operator<<(const char* _str) {
        string += _str;
        string += '\n';
        return *this;
    }
    ShaderSource& operator+=(const std::string _str) {
        string += _str;
        return *this;
    }
    ShaderSource& operator+=(const char* _str) {
        string += _str;
        return *this;
    }
    void clear() {
        string.clear();
    }
};
}
