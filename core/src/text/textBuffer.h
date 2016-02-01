#pragma once

#include "labels/labelMesh.h"
#include "style/textStyle.h"
#include "text/fontContext.h"

namespace Tangram {

class FontContext;

/*
 * This class holds TextLabels together with their VboMesh
 */
class TextBuffer : public LabelMesh, public LabelSet {

public:
    struct WordBreak {
        int start;
        int end;
    };

    class Builder {
        std::vector<Label::Vertex> m_scratchVertices;
        std::vector<Label::Vertex> m_vertices;
        std::vector<std::unique_ptr<Label>> m_labels;

        glm::vec2 m_bbox;
        glm::vec2 m_quadsLocalOrigin;
        int m_numLines;
        FontContext::FontMetrics m_metrics;

        std::unique_ptr<TextBuffer> m_mesh;

        std::vector<WordBreak> m_wordBreaks;

        int applyWordWrapping(std::vector<FONSquad>& _quads, const TextStyle::Parameters& _params,
                              const FontContext::FontMetrics& _metrics);

        static std::string applyTextTransform(const TextStyle::Parameters& _params, const std::string& _string);

    public:
        void setup(std::shared_ptr<VertexLayout> _vertexLayout);
        bool prepareLabel(FontContext& _fontContext, const TextStyle::Parameters& _params);
        void addLabel(const TextStyle::Parameters& _params, Label::Type _type,
                      Label::Transform _transform);

        std::unique_ptr<TextBuffer> build();

        float labelWidth() { return m_bbox.x; }

        static void findWords(const std::string& _text, std::vector<WordBreak>& _words);

    };

    TextBuffer(std::shared_ptr<VertexLayout> _vertexLayout);
    ~TextBuffer();

    virtual void draw(ShaderProgram& _shader) override;

protected:

    bool m_strokePass = false;

};

}
