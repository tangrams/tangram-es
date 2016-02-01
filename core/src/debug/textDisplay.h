#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <cstdio>
#include "gl/texture.h"
#include "gl/vboMesh.h"
#include "gl/shaderProgram.h"
#include "glm/vec2.hpp"

namespace Tangram {

#define LOG_CAPACITY        10
#define VERTEX_BUFFER_SIZE  99999

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

    /* Draw stacked messages added through log and draw _infos string list */
    void draw(const std::vector<std::string>& _infos);

    /* Stack the log message to be displayed in the screen log */
    void log(const char* fmt, ...);

private:

    TextDisplay();

    void draw(const std::string& _text, int _posx, int _posy);

    glm::vec2 m_textDisplayResolution;
    bool m_initialized;
    std::unique_ptr<ShaderProgram> m_shader;
    std::string m_log[LOG_CAPACITY];
    std::mutex m_mutex;
    char* m_vertexBuffer;
};

}
