#pragma once

#include "styleBatch.h"
#include "styleParamMap.h"
#include "text/textBuffer.h"
#include "data/tileData.h"

#include <vector>
#include <memory>

class Feature;
class MapTile;
class SpriteStyle;
class View;
class SpriteLabel;

struct Properties;

class SpriteBatch : public StyleBatch {

public:
    SpriteBatch(const SpriteStyle& _style);

    virtual void draw(const View& _view) override;
    virtual void update(const glm::mat4& mvp, const View& _view, float _dt) override;
    virtual void prepare() override;
    virtual bool compile() override;

    virtual void add(const Feature& _feature, const StyleParamMap& _params, const MapTile& _tile) override;

private:

    void buildPoint(const Point& _point, const Properties& _props, const MapTile& _tile);

    std::vector<std::shared_ptr<SpriteLabel>> m_labels;

    std::shared_ptr<TypedMesh<BufferVert>> m_mesh;
    const SpriteStyle& m_style;
};
