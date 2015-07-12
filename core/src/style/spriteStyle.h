#pragma once

#include "style.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "util/typedMesh.h"
#include "tile/labels/labels.h"
#include "tile/labels/spriteAtlas.h"

class Texture;
class SpriteLabel;

class SpriteStyle : public Style {

protected:
    
    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;

    virtual StyleBatch* newBatch() const override;

    std::unique_ptr<SpriteAtlas> m_spriteAtlas;
    std::shared_ptr<Labels> m_labels;
    size_t m_stateAttribOffset;

public:

    virtual void onBeginDrawFrame(const std::shared_ptr<View>& _view, const std::shared_ptr<Scene>& _scene) override;
    virtual void onEndDrawFrame() override;

    SpriteStyle(std::string _name, GLenum _drawMode = GL_TRIANGLES);

    virtual ~SpriteStyle();

    friend class SpriteBatch;
    
};

class SpriteBatch : public StyleBatch {

public:
    SpriteBatch(const SpriteStyle& _style);
    
    virtual void draw(const View& _view) override;
    virtual void update(const glm::mat4& mvp, const View& _view, float _dt) override;
    virtual void prepare() override;
    virtual bool compile() {
        if (m_mesh->numVertices() > 0) {
            m_mesh->compileVertexBuffer();
            return true;
        }
        return false;
    };

    virtual void add(const Feature& _feature, const StyleParamMap& _params, const MapTile& _tile) override;

private:

    void buildPoint(const Point& _point, const Properties& _props, const MapTile& _tile);

    std::vector<std::shared_ptr<SpriteLabel>> m_labels;

    std::shared_ptr<TypedMesh<BufferVert>> m_mesh;
    const SpriteStyle& m_style;
};

