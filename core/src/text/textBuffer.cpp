#include "textBuffer.h"
#include "labels/textLabel.h"

#include "gl/texture.h"
#include "gl/vboMesh.h"

namespace Tangram {

TextBuffer::TextBuffer(std::shared_ptr<VertexLayout> _vertexLayout)
    : LabelMesh(_vertexLayout, GL_TRIANGLES) {
    m_dirtyTransform = false;
    addVertices({}, {});
}

TextBuffer::~TextBuffer() {
}

bool TextBuffer::addLabel(const std::string& _text, Label::Transform _transform, Label::Type _type, const Parameters& _params, Label::Options _options) {
    if (_params.fontSize <= 0.f || _text.size() == 0) {
        return false;
    }

    auto fontContext = FontContext::GetInstance();

    if (!fontContext->lock()) {
        return false;
    }

    auto fontID = fontContext->getFontID(_params.fontKey);

    if(fontID < 0) {
        fontContext->unlock();
        return false;
    }

    const std::string* text = &_text;
    std::string transformedText;

    if (_params.transform != TextTransform::none) {
        transformedText = _text;
        std::locale loc;

        // perfom text transforms
        switch (_params.transform) {
            case TextTransform::capitalize:
                transformedText[0] = toupper(_text[0], loc);
                if (_text.size() > 1) {
                    for (std::string::size_type i = 1; i < _text.length(); ++i) {
                        if (_text[i - 1] == ' ') {
                            transformedText[i] = std::toupper(_text[i], loc);
                        }
                    }
                }
                break;
            case TextTransform::lowercase:
                for (std::string::size_type i = 0; i < _text.length(); ++i) {
                    transformedText[i] = std::tolower(_text[i], loc);
                }
                break;
            case TextTransform::uppercase:
                // TOOD : use to wupper when any wide character is detected
                for (std::string::size_type i = 0; i < _text.length(); ++i) {
                    transformedText[i] = std::toupper(_text[i], loc);
                }
                break;
            default:
                break;
        }

        text = &transformedText;
    }

    // rasterize glyphs
    std::vector<FONSquad>& quads = fontContext->rasterize(*text, fontID, _params.fontSize, _params.blurSpread);
    size_t numGlyphs = quads.size();

    if (numGlyphs == 0) {
        fontContext->unlock();
        return false;
    }

    auto& vertices = m_vertices[0];
    int vertexOffset = vertices.size();
    int numVertices = numGlyphs * 4;
    vertices.reserve(vertices.size() + numVertices);

    float inf = std::numeric_limits<float>::infinity();
    float x0 = inf, x1 = -inf, y0 = inf, y1 = -inf;

    for (auto& q : quads) {
        x0 = std::min(x0, std::min(q.x0, q.x1));
        x1 = std::max(x1, std::max(q.x0, q.x1));
        y0 = std::min(y0, std::min(q.y0, q.y1));
        y1 = std::max(y1, std::max(q.y0, q.y1));

        vertices.push_back({{q.x0, q.y0}, {q.s0, q.t0}, _options.color});
        vertices.push_back({{q.x0, q.y1}, {q.s0, q.t1}, _options.color});
        vertices.push_back({{q.x1, q.y0}, {q.s1, q.t0}, _options.color});
        vertices.push_back({{q.x1, q.y1}, {q.s1, q.t1}, _options.color});
    }

    fontContext->unlock();

    glm::vec2 size((x1 - x0), (y1 - y0));

    m_labels.emplace_back(new TextLabel(_text, _transform, _type, size, *this, { vertexOffset, numVertices }, _options));

    // TODO: change this in TypeMesh::adVertices()
    m_nVertices = vertices.size();

    return true;
}

}
