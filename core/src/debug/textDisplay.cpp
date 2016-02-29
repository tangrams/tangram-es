#include "textDisplay.h"
#include <cstdarg>
#include "platform.h"
#include "gl/vertexLayout.h"
#include "gl/renderState.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define STB_EASY_FONT_IMPLEMENTATION
#include "stb_easy_font.h"

namespace Tangram {

TextDisplay::TextDisplay() : m_textDisplayResolution(200.0), m_initialized(false) {
    m_vertexBuffer = new char[VERTEX_BUFFER_SIZE];
}

TextDisplay::~TextDisplay() {
    delete[] m_vertexBuffer;
}

void TextDisplay::init() {
    if (m_initialized) {
        return;
    }

    std::string vertShaderSrcStr = R"END(
        #ifdef GL_ES
        precision mediump float;
        #define LOWP lowp
        #else
        #define LOWP
        #endif
        uniform mat4 u_orthoProj;
        attribute vec2 a_position;
        void main() {
            gl_Position = u_orthoProj * vec4(a_position, 0.0, 1.0);
        }
    )END";
    std::string fragShaderSrcStr = R"END(
        #ifdef GL_ES
        precision mediump float;
        #define LOWP lowp
        #else
        #define LOWP
        #endif
        uniform vec3 u_color;
        void main(void) {
            gl_FragColor = vec4(u_color, 1.0);
        }
    )END";

    m_shader = std::make_unique<ShaderProgram>();
    m_shader->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);

    m_initialized = true;
}

void TextDisplay::log(const char* fmt, ...) {
    static char text[99999];

    va_list args;
    va_start(args, fmt);
    vsprintf(text, fmt, args);
    va_end(args);

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        for (int i = LOG_CAPACITY - 1; i >= 1; i--) {
            m_log[i] = m_log[i - 1];
        }

        m_log[0] = std::string(text);
    }
}

void TextDisplay::draw(const std::string& _text, int _posx, int _posy) {
    static VertexLayout vertexLayout({{"a_position", 2, GL_FLOAT, false, 0}});
    std::vector<glm::vec2> vertices;
    int nquads;

    nquads = stb_easy_font_print(_posx, _posy, _text.c_str(), NULL, m_vertexBuffer, VERTEX_BUFFER_SIZE);

    float* data = reinterpret_cast<float*>(m_vertexBuffer);

    vertices.reserve(nquads * 6);
    for (int quad = 0, stride = 0; quad < nquads; ++quad, stride += 16) {
        vertices.push_back({data[(0 * 4) + stride], data[(0 * 4) + 1 + stride]});
        vertices.push_back({data[(1 * 4) + stride], data[(1 * 4) + 1 + stride]});
        vertices.push_back({data[(2 * 4) + stride], data[(2 * 4) + 1 + stride]});
        vertices.push_back({data[(2 * 4) + stride], data[(2 * 4) + 1 + stride]});
        vertices.push_back({data[(3 * 4) + stride], data[(3 * 4) + 1 + stride]});
        vertices.push_back({data[(0 * 4) + stride], data[(0 * 4) + 1 + stride]});
    }
    vertexLayout.enable(*m_shader, 0, (void*)vertices.data());

    glDrawArrays(GL_TRIANGLES, 0, nquads * 6);
}

void TextDisplay::draw(const std::vector<std::string>& _infos) {
    GLint boundbuffer = -1;

    RenderState::culling(GL_FALSE);
    RenderState::blending(GL_FALSE);
    RenderState::depthTest(GL_FALSE);
    RenderState::depthWrite(GL_FALSE);

    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint*) &boundbuffer);
    RenderState::vertexBuffer(0);

    m_shader->use();

    glm::mat4 orthoProj = glm::ortho(0.f, (float)m_textDisplayResolution.x, (float)m_textDisplayResolution.y, 0.f, -1.f, 1.f);
    m_shader->setUniformMatrix4f(UniformEntries::getEntry(Uniform::orthoProj), orthoProj);

    // Display Tangram info messages
    m_shader->setUniformf(UniformEntries::getEntry(Uniform::color), 0.f, 0.f, 0.f);
    int offset = 0;
    for (auto& text : _infos) {
        draw(text, 3, 3 + offset);
        offset += 10;
    }

    // Display screen log
    offset = 0;
    m_shader->setUniformf(UniformEntries::getEntry(Uniform::color), 51 / 255.f, 73 / 255.f, 120 / 255.f);
    for (int i = 0; i < LOG_CAPACITY; ++i) {
        draw(m_log[i], 3, m_textDisplayResolution.y - 10 + offset);
        offset -= 10;
    }

    RenderState::culling(GL_TRUE);
    RenderState::vertexBuffer(boundbuffer);
}

}


