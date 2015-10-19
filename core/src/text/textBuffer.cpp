#include "textBuffer.h"

#include "labels/textLabel.h"
#include "gl/texture.h"
#include "gl/vboMesh.h"
#include "text/fontContext.h"

namespace Tangram {

TextBuffer::TextBuffer(std::shared_ptr<VertexLayout> _vertexLayout)
    : LabelMesh(_vertexLayout, GL_TRIANGLES) {
    addVertices({}, {});
}

TextBuffer::~TextBuffer() {
}

bool TextBuffer::addLabel(const TextStyle::Parameters& _params, Label::Transform _transform,
                          Label::Type _type, FontContext& _fontContext) {

    if (_params.fontId < 0 || _params.fontSize <= 0.f) {
        return false;
    }

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
                    for (size_t i = 1; i < text.length(); ++i) {
                        if (text[i - 1] == ' ') {
                            text[i] = std::toupper(text[i], loc);
                        }
                    }
                }
                break;
            case TextTransform::lowercase:
                for (size_t i = 0; i < text.length(); ++i) {
                    text[i] = std::tolower(text[i], loc);
                }
                break;
            case TextTransform::uppercase:
                // TOOD : use to wupper when any wide character is detected
                for (size_t i = 0; i < text.length(); ++i) {
                    text[i] = std::toupper(text[i], loc);
                }
                break;
            default:
                break;
        }
        renderText = &text;
    }

    if (!_fontContext.lock()) {
        return false;
    }

    // rasterize glyphs
    auto& quads = _fontContext.rasterize(*renderText, _params.fontId,
                                         _params.fontSize, _params.blurSpread);

    size_t numGlyphs = quads.size();

    if (numGlyphs == 0) {
        _fontContext.unlock();
        return false;
    }

    auto& vertices = m_vertices[0];
    int vertexOffset = vertices.size();
    int numVertices = numGlyphs * 4;

    float inf = std::numeric_limits<float>::infinity();
    float x0 = inf, x1 = -inf, y0 = inf, y1 = -inf;

    // Stroke width is normalized by the distance of the SDF spread, then scaled
    // to a char, then packed into the "alpha" channel of stroke. The .25 scaling
    // probably has to do with how the SDF is generated, but honestly I'm not sure
    // what it represents.
    uint32_t strokeWidth = _params.strokeWidth / _params.blurSpread * 255. * .25;
    uint32_t stroke = (_params.strokeColor & 0x00ffffff) + (strokeWidth << 24);

    for (auto& q : quads) {

        x0 = std::min(x0, std::min(q.x0, q.x1));
        x1 = std::max(x1, std::max(q.x0, q.x1));
        y0 = std::min(y0, std::min(q.y0, q.y1));
        y1 = std::max(y1, std::max(q.y0, q.y1));

        vertices.push_back({{q.x0, q.y0}, {q.s0, q.t0}, {-1.f, -1.f, 0.f}, _params.fill, stroke});
        vertices.push_back({{q.x0, q.y1}, {q.s0, q.t1}, {-1.f,  1.f, 0.f}, _params.fill, stroke});
        vertices.push_back({{q.x1, q.y0}, {q.s1, q.t0}, { 1.f, -1.f, 0.f}, _params.fill, stroke});
        vertices.push_back({{q.x1, q.y1}, {q.s1, q.t1}, { 1.f,  1.f, 0.f}, _params.fill, stroke});
    }

    _fontContext.unlock();

    glm::vec2 size((x1 - x0), (y1 - y0));

    m_labels.emplace_back(new TextLabel(_transform, _type, size, *this,
                                        { vertexOffset, numVertices },
                                        _params.labelOptions));

    // TODO: change this in TypeMesh::adVertices()
    m_nVertices = vertices.size();

    return true;
}

}
