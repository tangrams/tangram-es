#pragma once

#include "style.h"
#include "fontstash/glfontstash.h"
#include "text/fontContext.h"
#include "label.h"
#include "stl_util.hpp"
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

struct Atlas {
    TextureData m_data;
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

    FontStyle(const std::string& _fontFile, std::string _name, float _fontSize, GLenum _drawMode = GL_TRIANGLES);

    virtual void setup(View& _view) override;
    virtual void teardown() override;
    virtual void setupForTile(const MapTile& _tile) override;

    virtual ~FontStyle();

    /* 
     * fontstash callbacks, non thread-safe
     */

    /* Called by fontstash when the texture need to create a new transform textures */
    friend void createTexTransforms(void* _userPtr, unsigned int _width, unsigned int _height);

    /* Called by fontsash when the texture need to be updated */
    friend void updateTransforms(void* _userPtr, unsigned int _xoff, unsigned int _yoff, unsigned int _width,
                            unsigned int _height, const unsigned int* _pixels, void* _ownerPtr);

    /* Called by fontstash when the atlas need to update the atlas texture */
    friend void updateAtlas(void* _userPtr, unsigned int _xoff, unsigned int _yoff,
                            unsigned int _width, unsigned int _height, const unsigned int* _pixels);

    /*
     * fontstash callbacks, thread-safe
     */

    /* Called by fontstash when the atlas need to be created */
    friend void createAtlas(void* _usrPtr, unsigned int _width, unsigned int _height);

    /* Callback on errors */
    friend void errorCallback(void* _userPtr, fsuint buffer, GLFONSError error);

    GLuint textureTransformName(const TileID _tileId) const;

private:

    void processTileTransformCreation();
    void processTileTransformUpdate();
    void processAtlasUpdate();

    void initFontContext(const std::string& _fontFile);

    float m_fontSize;
    int m_font;

    std::map<TileID, GLuint> m_tileTexTransforms;

    /* Since the fontstash callbacks are called from threads, we enqueue them */
    std::queue<TileTransform> m_pendingTexTransformsData;
    std::queue<Atlas> m_pendingTexAtlasData;
    std::queue<std::pair<TileID, glm::vec2>> m_pendingTileTexTransforms;

    // pointer to the currently processed tile by build* methods, nullptr if not
    MapTile* m_processedTile;
    GLuint m_atlas;
    std::shared_ptr<FontContext> m_fontContext;
};
