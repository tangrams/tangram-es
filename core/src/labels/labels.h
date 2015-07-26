#pragma once

#include "label.h"
#include "textLabel.h"
#include "spriteLabel.h"
#include "tile/tileID.h"

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

    void setView(std::shared_ptr<View> _view) { m_view = _view; }

    void drawDebug();

    void update(float _dt, const std::vector<std::unique_ptr<Style>>& _styles,
                const std::map<TileID, std::shared_ptr<Tile>>& _tiles);

    bool needUpdate() { return m_needUpdate; }

private:

    int LODDiscardFunc(float _maxZoom, float _zoom);

    std::shared_ptr<View> m_view;
    bool m_needUpdate;

    // temporary data used in update()
    std::vector<Label*> m_labels;
    std::vector<isect2d::AABB> m_aabbs;
};

}
