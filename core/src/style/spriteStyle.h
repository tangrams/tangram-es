#pragma once

#include "style.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "util/typedMesh.h"
#include "tile/labels/labels.h"
#include "tile/labels/spriteAtlas.h"

class Texture;

class SpriteStyle : public Style {

protected:

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void buildPoint(Point& _point, const StyleParamMap& _styleParamMap, Properties& _props, Batch& _batch, MapTile& _tile) const override;

    typedef TypedMesh<BufferVert> Mesh;

    virtual Batch* newBatch() const override;
    
    std::unique_ptr<SpriteAtlas> m_spriteAtlas;
    std::shared_ptr<Labels> m_labels;
    unsigned int m_spriteGeneration;
    
public:

    virtual void onBeginDrawFrame(const std::shared_ptr<View>& _view, const std::shared_ptr<Scene>& _scene) override;
    virtual void onEndDrawFrame() override;
    
    void setSpriteAtlas(std::unique_ptr<SpriteAtlas> _atlas);

    SpriteStyle(std::string _name, GLenum _drawMode = GL_TRIANGLES);

    virtual ~SpriteStyle();

    friend class SpriteBatch;
    
};

class SpriteBatch : public Batch {
public:
    SpriteBatch(const SpriteStyle& _style);
    
    virtual void draw(const View& _view) override;
    virtual void update(float _dt, const View& _view) override {};
    virtual bool compile() {
        if (m_mesh->numVertices() > 0) {
            m_mesh->compileVertexBuffer();
            return true;
        }
        return false;
    };

    std::shared_ptr<TypedMesh<BufferVert>> m_mesh;
    const SpriteStyle& m_style;
};

