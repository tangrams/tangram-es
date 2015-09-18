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

Label::Options optionsFromTextParams(const Parameters& _params) {
    Label::Options options;
    options.color = _params.fill;
    options.priority = _params.priority;
    options.offset = _params.offset;

    options.interactive = _params.interactive;
    if (options.interactive) {
        options.properties = _params.properties;
    }

    return options;
}

bool TextBuffer::addLabel(const Parameters& _params, Label::Transform _transform, Label::Type _type) {

    auto options = optionsFromTextParams(_params);

    const std::string* renderText;
    std::string text;

    if (_params.transform == TextTransform::none) {
        renderText = &_params.text;
    } else {
        text = _params.text;
        std::locale loc;

        // perfom text transforms
        switch (_params.transform) {
            case TextTransform::capitalize:
                text[0] = toupper(text[0], loc);
                if (text.size() > 1) {
                    for (auto i = 1; i < text.length(); ++i) {
                        if (text[i - 1] == ' ') {
                            text[i] = std::toupper(text[i], loc);
                        }
                    }
                }
                break;
            case TextTransform::lowercase:
                for (auto i = 0; i < text.length(); ++i) {
                    text[i] = std::tolower(text[i], loc);
                }
                break;
            case TextTransform::uppercase:
                // TOOD : use to wupper when any wide character is detected
                for (auto i = 0; i < text.length(); ++i) {
                    text[i] = std::toupper(text[i], loc);
                }
                break;
            default:
                break;
        }
        renderText = &text;
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

    // rasterize glyphs
    std::vector<FONSquad>& quads = fontContext->rasterize(*renderText, fontID,
                                                          _params.fontSize,
                                                          _params.blurSpread);
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

        vertices.push_back({{q.x0, q.y0}, {q.s0, q.t0}, options.color});
        vertices.push_back({{q.x0, q.y1}, {q.s0, q.t1}, options.color});
        vertices.push_back({{q.x1, q.y0}, {q.s1, q.t0}, options.color});
        vertices.push_back({{q.x1, q.y1}, {q.s1, q.t1}, options.color});
    }

    fontContext->unlock();

    glm::vec2 size((x1 - x0), (y1 - y0));

    m_labels.emplace_back(new TextLabel(_params.text, _transform, _type, size, *this,
                                        { vertexOffset, numVertices }, options));

    // TODO: change this in TypeMesh::adVertices()
    m_nVertices = vertices.size();

    return true;
}

}
