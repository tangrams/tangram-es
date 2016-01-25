#include "textDisplay.h"
#include "platform.h"
#include "gl/vertexLayout.h"
#include "gl/renderState.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "stb_easy_font.h"

namespace Tangram {

TextDisplay::TextDisplay(glm::vec2 _textDisplayResolution)
    : m_textDisplayResolution(_textDisplayResolution)
{
}

TextDisplay::~TextDisplay() {}

void TextDisplay::init() {
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
        void main(void) {
            gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        }
    )END";

    m_shader = std::make_unique<ShaderProgram>();
    m_shader->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);
}

void TextDisplay::draw(std::vector<std::string> _infos) {
    static std::shared_ptr<VertexLayout> vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 2, GL_FLOAT, false, 0},
    }));

    static char buffer[99999];
    static GLint boundbuffer = -1;

    RenderState::culling(GL_FALSE);
    RenderState::blending(GL_FALSE);
    RenderState::depthTest(GL_FALSE);
    RenderState::depthWrite(GL_FALSE);

    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint*) &boundbuffer);
    RenderState::vertexBuffer(0);

    m_shader->use();

    glm::mat4 orthoProj = glm::ortho(0.f, (float)m_textDisplayResolution.x, (float)m_textDisplayResolution.y, 0.f, -1.f, 1.f);
    m_shader->setUniformMatrix4f("u_orthoProj", orthoProj);

    int offset = 0;

    for (auto& text : _infos) {
        std::vector<glm::vec2> vertices;
        int nquads;

        nquads = stb_easy_font_print(3, 3 + offset, text.c_str(), NULL, buffer, sizeof(buffer));

        float* data = reinterpret_cast<float*>(buffer);

        vertices.reserve(nquads * 6);
        for (int quad = 0, stride = 0; quad < nquads; ++quad, stride += 16) {
            vertices.push_back({data[(0 * 4) + stride], data[(0 * 4) + 1 + stride]});
            vertices.push_back({data[(1 * 4) + stride], data[(1 * 4) + 1 + stride]});
            vertices.push_back({data[(2 * 4) + stride], data[(2 * 4) + 1 + stride]});
            vertices.push_back({data[(2 * 4) + stride], data[(2 * 4) + 1 + stride]});
            vertices.push_back({data[(3 * 4) + stride], data[(3 * 4) + 1 + stride]});
            vertices.push_back({data[(0 * 4) + stride], data[(0 * 4) + 1 + stride]});
        }
        vertexLayout->enable(*m_shader, 0, (void*)vertices.data());

        glDrawArrays(GL_TRIANGLES, 0, nquads * 6);

        offset += 10;
    }

    RenderState::culling(GL_TRUE);
    RenderState::vertexBuffer(boundbuffer);
}

}


