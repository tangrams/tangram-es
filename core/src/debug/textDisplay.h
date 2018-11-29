#pragma once

#include "gl/texture.h"
#include "gl/mesh.h"
#include "gl/shaderProgram.h"
#include "glm/vec2.hpp"
#include <string>
#include <vector>
#include <mutex>
#include <cstdio>
#include <sstream>
#include <iomanip>

namespace Tangram {

#define LOG_CAPACITY        20
#define VERTEX_BUFFER_SIZE  99999

typedef int FontID;
class RenderState;

namespace Debug {

template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 6) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(n) << a_value;
    return out.str();
}

class TextDisplay {

public:

    static TextDisplay& Instance() {
        static TextDisplay instance;
        instance.init();
        return instance;
    }


#ifdef TANGRAM_DEBUG_RENDERER
    ~TextDisplay();
    void setResolution(glm::vec2 _textDisplayResolution) { m_textDisplayResolution = _textDisplayResolution; }

    void init();
    void deinit();

    /* Draw stacked messages added through log and draw _infos string list */
    void draw(RenderState& rs, const std::vector<std::string>& _infos);
    /* Stack the log message to be displayed in the screen log */
    void log(const char* fmt, ...);

private:
    TextDisplay();
    void draw(RenderState& rs, const std::string& _text, int _posx, int _posy);

    glm::vec2 m_textDisplayResolution;
    bool m_initialized;
    std::unique_ptr<ShaderProgram> m_shader;
    std::string m_log[LOG_CAPACITY];
    std::mutex m_mutex;
    char* m_vertexBuffer = nullptr;

    UniformLocation m_uOrthoProj{"u_orthoProj"};
    UniformLocation m_uColor{"u_color"};
#else
    ~TextDisplay() {}
    void setResolution(glm::vec2) {}
    void init() {}
    void deinit() {}
    void draw(RenderState&, const std::vector<std::string>&) {}
    void log(const char* fmt, ...) {}
private:
    TextDisplay(){}
#endif
};
}

#ifdef TANGRAM_DEBUG_RENDERER
#define LOGS(fmt, ...) \
    do { Tangram::Debug::TextDisplay::Instance().log(fmt, ## __VA_ARGS__); } while(0)
#else
#define LOGS(...)
#endif

}
