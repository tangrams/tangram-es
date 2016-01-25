#pragma once

#include <string>
#include <vector>
#include <mutex>
#include "gl/texture.h"
#include "gl/typedMesh.h"
#include "gl/shaderProgram.h"
#include "glm/vec2.hpp"

namespace Tangram {

#define LOG_CAPACITY 10

typedef int FontID;

class TextDisplay {
public:
    static TextDisplay& Instance() {
        static TextDisplay instance;
        instance.init();
        return instance;
    }

    ~TextDisplay();

    void setResolution(glm::vec2 _textDisplayResolution) { m_textDisplayResolution = _textDisplayResolution; }

    void init();

    void draw(std::vector<std::string> _infos);
    void log(const char* fmt, ...);

private:
    TextDisplay();

    void draw(const std::string& _text, int _posx, int _posy);

    bool m_initialized;
    std::unique_ptr<ShaderProgram> m_shader;
    glm::vec2 m_textDisplayResolution;
    std::string m_log[LOG_CAPACITY];
    std::mutex m_mutex;
};

}
