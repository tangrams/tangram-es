#pragma once

#include "label.h"
#include "spriteLabel.h"
#include "tile/tileID.h"
#include "data/properties.h"
#include "isect2d.h"
#include "glm_vec.h" // for isect2d.h

#include <memory>
#include <mutex>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>

#define PERF_TRACE __attribute__ ((noinline))

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

    void updateLabelSet(const View& _view, float _dt, const std::vector<std::unique_ptr<Style>>& _styles,
                        const std::vector<std::shared_ptr<Tile>>& _tiles, std::unique_ptr<TileCache>& _cache);

    PERF_TRACE void updateLabels(const View& _view, float _dt, const std::vector<std::unique_ptr<Style>>& _styles,
                                 const std::vector<std::shared_ptr<Tile>>& _tiles, bool _onlyTransitions = true);

    const std::vector<TouchItem>& getFeaturesAtPoint(const View& _view, float _dt,
                                                     const std::vector<std::unique_ptr<Style>>& _styles,
                                                     const std::vector<std::shared_ptr<Tile>>& _tiles,
                                                     float _x, float _y, bool _visibleOnly = true);

    bool needUpdate() const { return m_needUpdate; }

private:

    using AABB = isect2d::AABB<glm::vec2>;
    using OBB = isect2d::OBB<glm::vec2>;
    using CollisionPairs = std::vector<isect2d::ISect2D<glm::vec2>::Pair>;


    void skipTransitions(const std::vector<std::unique_ptr<Style>>& _styles,
                         const std::vector<std::shared_ptr<Tile>>& _tiles,
                         std::unique_ptr<TileCache>& _cache, float _currentZoom) const;

    PERF_TRACE void skipTransitions(const std::vector<const Style*>& _styles, Tile& _tile, Tile& _proxy) const;

    PERF_TRACE void sortLabels();

    PERF_TRACE void handleOcclusions();

    PERF_TRACE bool withinRepeatDistance(Label *_label);

    bool m_needUpdate;

    isect2d::ISect2D<glm::vec2> m_isect2d;

    std::vector<TouchItem> m_touchItems;

    struct LabelEntry {

        LabelEntry(Label* _label, bool _proxy)
            : label(_label),
              priority(_label->options().priority),
              proxy(_proxy) {}

        Label* label;

        float priority;
        bool proxy;
    };

    static bool labelComparator(const LabelEntry& _a, const LabelEntry& _b);

    std::vector<LabelEntry> m_labels;

    std::unordered_map<size_t, std::vector<Label*>> m_repeatGroups;

    float m_lastZoom;
};

}

