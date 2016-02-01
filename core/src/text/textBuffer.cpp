#include "textBuffer.h"

#include "labels/textLabel.h"
#include "gl/texture.h"
#include "gl/vboMesh.h"
#include "gl/shaderProgram.h"

#include <limits>
#include <memory>
#include <locale>

namespace Tangram {

TextBuffer::TextBuffer(std::shared_ptr<VertexLayout> _vertexLayout)
    : LabelMesh(_vertexLayout, GL_TRIANGLES) {}

TextBuffer::~TextBuffer() {
}

void TextBuffer::Builder::findWords(const std::string& _text, std::vector<WordBreak>& _wordBreaks) {

    unsigned int utf8state = 0;
    const char* str = _text.c_str();
    const char* end = str + strlen(str);
    int wordBegin = 0;
    int wordEnd = -1;
    int itr = 0;

    for (; str != end; ++str) {
        unsigned int byte = *(const unsigned char*)str;

        if (fonsDecUTF8(&utf8state, byte)) {
            continue;
        }

        if (std::isspace(*str)) {
            wordEnd = itr - 1;
            if (wordBegin <= wordEnd) {
                _wordBreaks.push_back({wordBegin, wordEnd});
            }
            if (*str == '\n') {
                _wordBreaks.push_back({itr, itr});
            }
            wordBegin = itr + 1;
        }
        itr++;
    }

    if (!std::isspace(*(end-1))) {
        _wordBreaks.push_back({wordBegin, itr-1});
    }
}

int TextBuffer::Builder::applyWordWrapping(std::vector<FONSquad>& _quads,
                                           const TextStyle::Parameters& _params,
                                           const FontContext::FontMetrics& _metrics) {
    struct LineQuad {
        std::vector<FONSquad*> quads;
        float length = 0.0f;
    };

    float yOffset = 0.f, xOffset = 0.f;
    int nLine = 1;

    std::vector<LineQuad> lines;
    if (_params.wordWrap && _params.maxLineWidth < _params.text.length()) {
        findWords(_params.text, m_wordBreaks);
    } else {
        return 1;
    }

    lines.push_back(LineQuad()); // atleast one line
    float totalWidth = 0.f;

    // Apply word wrapping based on the word breaks
    for (int iWord = 0; iWord < int(m_wordBreaks.size()); iWord++) {
        int start = m_wordBreaks[iWord].start;
        int end = m_wordBreaks[iWord].end;
        size_t wordSize = end - start + 1;

        auto& lastLineQuads = lines[nLine - 1].quads;

        // Check if quads need to be added to next line?
        if (iWord > 0 && (lastLineQuads.size() + wordSize) > size_t(_params.maxLineWidth)) {
            xOffset = 0.0f;
            auto& quad = _quads[start];
            auto& prevQuad = lines[nLine - 1].quads.front();

            float baseLength = quad.x0 - prevQuad->x0;
            yOffset += _metrics.lineHeight;
            xOffset -= (baseLength);
            lines.push_back(LineQuad());
            nLine++;
        }

        for (int i = start; i <= end; i++) {
            auto& q = _quads[i];

            q.x0 += xOffset;
            q.x1 += xOffset;
            q.y0 += yOffset;
            q.y1 += yOffset;

            lines[nLine - 1].quads.push_back(&q);
            lines[nLine - 1].length = q.x1;

            totalWidth = std::max(totalWidth, q.x1);
        }
    }

    // Apply justification
    for (const auto& line : lines) {
        float paddingRight = totalWidth - line.length;
        float padding = 0;

        switch(_params.align) {
            case TextLabelProperty::Align::left: padding = 0.f; break;
            case TextLabelProperty::Align::right: padding = paddingRight; break;
            case TextLabelProperty::Align::center: padding = paddingRight * 0.5f; break;
        }

        for (auto quad : line.quads) {
            quad->x0 += padding;
            quad->x1 += padding;
        }
    }

    return nLine;
}


std::string TextBuffer::Builder::applyTextTransform(const TextStyle::Parameters& _params,
                                                    const std::string& _string) {

    std::locale loc;
    std::string text = _string;

    // perfom text transforms
    switch (_params.transform) {
        case TextLabelProperty::Transform::capitalize:
            text[0] = toupper(text[0], loc);
            if (text.size() > 1) {
                for (size_t i = 1; i < text.length(); ++i) {
                    if (text[i - 1] == ' ') {
                        text[i] = std::toupper(text[i], loc);
                    }
                }
            }
            break;
        case TextLabelProperty::Transform::lowercase:
            for (size_t i = 0; i < text.length(); ++i) {
                text[i] = std::tolower(text[i], loc);
            }
            break;
        case TextLabelProperty::Transform::uppercase:
            // TODO : use to wupper when any wide character is detected
            for (size_t i = 0; i < text.length(); ++i) {
                text[i] = std::toupper(text[i], loc);
            }
            break;
        default:
            break;
    }

    return text;
}

void TextBuffer::draw(ShaderProgram& _shader) {

    if (m_strokePass) {
        _shader.setUniformi("u_pass", 1);
        LabelMesh::draw(_shader);
        _shader.setUniformi("u_pass", 0);
    }

    LabelMesh::draw(_shader);
}

void TextBuffer::Builder::setup(std::shared_ptr<VertexLayout> _vertexLayout) {
    m_mesh = std::make_unique<TextBuffer>(_vertexLayout);
    m_vertices.clear();
    m_labels.clear();
}

std::unique_ptr<TextBuffer> TextBuffer::Builder::build() {
    if (!m_labels.empty()) {
        m_mesh->setLabels(m_labels);
        m_mesh->compile(m_vertices);
    }
    return std::move(m_mesh);
}

bool TextBuffer::Builder::prepareLabel(FontContext& _fontContext,
                                       const TextStyle::Parameters& _params) {

    if (_params.fontId < 0 || _params.fontSize <= 0.f || _params.text.size() == 0) {
        return false;
    }

    /// Apply text transforms
    const std::string* renderText;
    std::string text;

    if (_params.transform == TextLabelProperty::Transform::none) {
        renderText = &_params.text;
    } else {
        text = applyTextTransform(_params, _params.text);
        renderText = &text;
    }

    if (!_fontContext.lock()) {
        return false;
    }

    /// Rasterize the glyphs
    auto& quads = _fontContext.rasterize(*renderText, _params.fontId,
                                         _params.fontSize, _params.blurSpread);

    size_t numGlyphs = quads.size();

    if (numGlyphs == 0) {
        _fontContext.unlock();
        return false;
    }

    // Stroke width is normalized by the distance of the SDF spread, then scaled
    // to a char, then packed into the "alpha" channel of stroke. The .25 scaling
    // probably has to do with how the SDF is generated, but honestly I'm not sure
    // what it represents.
    uint32_t strokeWidth = _params.strokeWidth / _params.blurSpread * 255. * .25;
    uint32_t stroke = (_params.strokeColor & 0x00ffffff) + (strokeWidth << 24);

    m_metrics = _fontContext.getMetrics();
    m_bbox = glm::vec2(0);

    /// Apply word wrapping
    int nLine = 1;

    m_wordBreaks.clear();
    nLine = applyWordWrapping(quads, _params, m_metrics);

    /// Generate the quads
    float yMin = std::numeric_limits<float>::max();
    float xMin = std::numeric_limits<float>::max();

    m_scratchVertices.clear();

    for (int i = 0; i < int(quads.size()); ++i) {
        if (m_wordBreaks.size() > 0) {
            bool skip = false;
            // Skip spaces/CR quads
            for (int j = 0; j < int(m_wordBreaks.size()) - 1; ++j) {
                const auto& b1 = m_wordBreaks[j];
                const auto& b2 = m_wordBreaks[j + 1];
                if (i >= b1.end + 1 && i <= b2.start - 1) {
                    skip = true;
                    break;
                }
            }
            if (skip) { continue; }
        }

        const auto& q = quads[i];
        m_scratchVertices.push_back({{q.x0, q.y0}, {q.s0, q.t0}, _params.fill, stroke});
        m_scratchVertices.push_back({{q.x0, q.y1}, {q.s0, q.t1}, _params.fill, stroke});
        m_scratchVertices.push_back({{q.x1, q.y0}, {q.s1, q.t0}, _params.fill, stroke});
        m_scratchVertices.push_back({{q.x1, q.y1}, {q.s1, q.t1}, _params.fill, stroke});

        yMin = std::min(yMin, q.y0);
        xMin = std::min(xMin, q.x0);

        m_bbox.x = std::max(m_bbox.x, q.x1);
        m_bbox.y = std::max(m_bbox.y, std::abs(yMin - q.y1));
    }

    m_bbox.x -= xMin;
    m_quadsLocalOrigin = { xMin, quads[0].y0 };
    m_numLines = nLine;

    _fontContext.unlock();

    return true;
}

void TextBuffer::Builder::addLabel(const TextStyle::Parameters& _params, Label::Type _type,
                                   Label::Transform _transform) {

    int vertexOffset = m_vertices.size();
    int numVertices = m_scratchVertices.size();

    if (_params.strokeWidth > 0.f) { m_mesh->m_strokePass = true; }

    m_labels.emplace_back(new TextLabel(_transform, _type, m_bbox, *m_mesh,
                                        { vertexOffset, numVertices },
                                        _params.labelOptions, m_metrics, m_numLines,
                                        _params.anchor, m_quadsLocalOrigin));

    m_vertices.insert(m_vertices.end(),
                      m_scratchVertices.begin(),
                      m_scratchVertices.end());
}

}
