#pragma once

#include "gl.h"
#include "glfontstash.h"

#include "util/typedMesh.h"
#include "style/style.h"
#include "text/textBuffer.h"

#include "glm/vec4.hpp"
#include "glm/vec2.hpp"

#include "style/styleBatch.h"

#include <memory>

class FontContext;
class TextStyle;
class TextLabel;

/*
 * This class represents a text buffer, each text buffer has several text ids
 */
class TextBatch : public StyleBatch {

public:

    TextBatch(const TextStyle& _style);

    virtual void draw(const View& _view) override;
    virtual void update(const glm::mat4& mvp, const View& _view, float _dt) override;
    virtual void prepare() override;
    virtual bool compile() override;

    virtual void add(const Feature& _feature, const StyleParamMap& _params, const MapTile& _tile) override;

    ~TextBatch();

    /* generates a text id */
    fsuint genTextID();

    /* creates a text buffer and bind it */
    void init();

    /* ask the font rasterizer to rasterize a specific text for a text id */
    bool rasterize(const std::string& _text, fsuint _id);

    /*
     * transform a text id in screen space coordinate
     *  x, y in screen space
     *  rotation is in radians
     *  alpha should be in [0..1]
     */
    void transformID(fsuint _textID, const BufferVert::State& _state);

    void pushBuffer();

    int getVerticesSize();

    /* get the axis aligned bounding box for a text */
    glm::vec4 getBBox(fsuint _textID);

    void addLabel(std::shared_ptr<TextLabel> _label) {
        if (_label) { m_labels.push_back(std::move(_label)); }
    }

private:

    void buildPoint(const Point& _point, const Properties& _props, const MapTile& _tile);
    void buildLine(const Line& _line, const Properties& _props, const MapTile& _tile);
    void buildPolygon(const Polygon& _polygon, const Properties& _props, const MapTile& _tile);

    bool m_dirtyTransform;
    fsuint m_fsBuffer;
    std::shared_ptr<FontContext> m_fontContext;

    std::vector<std::shared_ptr<TextLabel>> m_labels;

    std::shared_ptr<TypedMesh<BufferVert>> m_mesh;

    const TextStyle& m_style;
};
