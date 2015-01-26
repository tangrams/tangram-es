#pragma once

#include "style.h"
#include "glfontstash.h"
#include "text/fontContext.h"
#include "label.h"
#include "stl_util.hpp"
#include "texture.h"
#include <map>
#include <memory>
#include <queue>
#include <mutex>

struct TextureData {
    const unsigned int* m_pixels;
    unsigned int m_xoff;
    unsigned int m_yoff;
    unsigned int m_width;
    unsigned int m_height;
};

struct TileTransform {
    TileID m_id;
    TextureData m_data;
};

class FontStyle : public Style {

protected:

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void buildPoint(Point& _point, std::string& _layer, Properties& _props, VboMesh& _mesh) override;
    virtual void buildLine(Line& _line, std::string& _layer, Properties& _props, VboMesh& _mesh) override;
    virtual void buildPolygon(Polygon& _polygon, std::string& _layer, Properties& _props, VboMesh& _mesh) override;
    virtual void prepareDataProcessing(MapTile& _tile) override;
    virtual void finishDataProcessing(MapTile& _tile) override;

public:

    FontStyle(const std::string& _fontFile, std::string _name, float _fontSize, bool _sdf = false, GLenum _drawMode = GL_TRIANGLES);

    virtual void setupFrame(const std::shared_ptr<View>& _view) override;
    virtual void teardown() override;

    virtual ~FontStyle();

private:

    float m_fontSize;
    bool m_sdf;

    // pointer to the currently processed tile by build* methods, nullptr if not
    MapTile* m_processedTile;
    std::unique_ptr<Texture> m_atlas;
    std::shared_ptr<FontContext> m_fontContext;
};
