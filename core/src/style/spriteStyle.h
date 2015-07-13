#pragma once

#include "style.h"

class Labels;
class Texture;
class SpriteLabel;
class SpriteBatch;
class SpriteAtlas;
class StyleBatch;

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
