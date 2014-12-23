#pragma once

#include "style.h"
#include "fontstash/glfontstash.h"
#include <map>
#include <mutex>

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

    FontStyle(const std::string& _fontFile, std::string _name, GLenum _drawMode = GL_TRIANGLES);

    virtual void setup() override;

    virtual ~FontStyle();

    friend void createTexTransforms(void* _userPtr, unsigned int _width, unsigned int _height);
    friend void updateTransforms(void* _userPtr, unsigned int _xoff, unsigned int _yoff,
                            unsigned int _width, unsigned int _height, const unsigned int* _pixels);
    friend void updateAtlas(void* _userPtr, unsigned int _xoff, unsigned int _yoff,
                            unsigned int _width, unsigned int _height, const unsigned int* _pixels);
    friend void createAtlas(void* _usrPtr, unsigned int _width, unsigned int _height);

private:

    void initFontContext(const std::string& _fontFile);

    int m_font;
    std::map<TileID, fsuint> m_tileBuffers;
    FONScontext* m_fontContext;

    std::mutex m_buildMutex;
};
