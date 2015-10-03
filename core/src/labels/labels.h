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

namespace Tangram {

class FontContext;
class Tile;
class View;
class Style;

/*
 * Singleton class containing all labels
 */

class Labels {

public:
    Labels();

    virtual ~Labels();

    void drawDebug(const View& _view);

    void update(const View& _view, float _dt, const std::vector<std::unique_ptr<Style>>& _styles,
                const std::vector<std::shared_ptr<Tile>>& _tiles);

    const std::vector<std::shared_ptr<Properties>>& getFeaturesAtPoint(const View& _view, float _dt,
                                                                       const std::vector<std::unique_ptr<Style>>& _styles,
                                                                       const std::vector<std::shared_ptr<Tile>>& _tiles,
                                                                       float _x, float _y, bool _visibleOnly = true);

    bool needUpdate() { return m_needUpdate; }

private:

    int LODDiscardFunc(float _maxZoom, float _zoom);

    bool m_needUpdate;

    using AABB = isect2d::AABB<glm::vec2>;
    using OBB = isect2d::OBB<glm::vec2>;

    // temporary data used in update()
    std::vector<Label*> m_labels;
    std::vector<AABB> m_aabbs;

    isect2d::ISect2D<glm::vec2> m_isect2d;

    std::vector<std::shared_ptr<Properties>> m_touchItems;

    OBB m_touchPoint{0,0,0,0,0};
};

}
