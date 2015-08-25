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

    void drawDebug(const View& _view);

    void update(const View& _view, float _dt, const std::vector<std::unique_ptr<Style>>& _styles,
                const std::vector<std::shared_ptr<Tile>>& _tiles);

    bool needUpdate() { return m_needUpdate; }

private:

    int LODDiscardFunc(float _maxZoom, float _zoom);

    bool m_needUpdate;

    // temporary data used in update()
    std::vector<Label*> m_labels;
    std::vector<isect2d::AABB> m_aabbs;
};

}
