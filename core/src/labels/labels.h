#pragma once

#include "label.h"
#include "textLabel.h"
#include "spriteLabel.h"
#include "tile/tileID.h"
#include "data/properties.h"
#include "isect2d.h"
#include "glm_vec.h" // for isect2d.h

#include <memory>
#include <mutex>
#include <vector>
#include <map>
#include <set>

namespace Tangram {

class FontContext;
class Tile;
class View;
class Style;
class TileCache;
struct TouchItem;

class Labels {

public:
    Labels();

    virtual ~Labels();

    void drawDebug(const View& _view);

    void update(const View& _view, float _dt, const std::vector<std::unique_ptr<Style>>& _styles,
                const std::vector<std::shared_ptr<Tile>>& _tiles, std::unique_ptr<TileCache>& _cache);

    const std::vector<TouchItem>& getFeaturesAtPoint(const View& _view, float _dt,
                                                     const std::vector<std::unique_ptr<Style>>& _styles,
                                                     const std::vector<std::shared_ptr<Tile>>& _tiles,
                                                     float _x, float _y, bool _visibleOnly = true);

    bool needUpdate() const { return m_needUpdate; }

private:

    using AABB = isect2d::AABB<glm::vec2>;
    using OBB = isect2d::OBB<glm::vec2>;
    using CollisionPairs = std::vector<isect2d::ISect2D<glm::vec2>::Pair>;

    void updateLabels(const std::vector<std::unique_ptr<Style>>& _styles,
                      const std::vector<std::shared_ptr<Tile>>& _tiles,
                      float _dt, float _dz, const View& _view);

    void skipTransitions(const std::vector<std::unique_ptr<Style>>& _styles,
                         const std::vector<std::shared_ptr<Tile>>& _tiles,
                         std::unique_ptr<TileCache>& _cache, float _currentZoom) const;

    void checkRepeatGroups(std::vector<TextLabel*>& _visibleSet) const;

    int LODDiscardFunc(float _maxZoom, float _zoom);

    bool m_needUpdate;

    // temporary data used in update()
    std::vector<Label*> m_labels;
    std::vector<AABB> m_aabbs;

    isect2d::ISect2D<glm::vec2> m_isect2d;

    std::vector<TouchItem> m_touchItems;

    float m_lastZoom;
};

}

