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

#ifdef _MSC_VER
#define PERF_TRACE
#else
#define PERF_TRACE __attribute__ ((noinline))
#endif

namespace Tangram {

class FontContext;
class Marker;
class Tile;
class View;
class Style;
class TileCache;
struct TouchItem;

class Labels {

public:
    Labels();

    virtual ~Labels();

    void drawDebug(RenderState& rs, const View& _view);

    void updateLabelSet(const View& _view, float _dt,
                        const std::vector<std::unique_ptr<Style>>& _styles,
                        const std::vector<std::shared_ptr<Tile>>& _tiles,
                        const std::vector<std::unique_ptr<Marker>>& _markers,
                        TileCache& _cache);

    PERF_TRACE void updateLabels(const View& _view, float _dt,
                                 const std::vector<std::unique_ptr<Style>>& _styles,
                                 const std::vector<std::shared_ptr<Tile>>& _tiles,
                                 const std::vector<std::unique_ptr<Marker>>& _markers,
                                 bool _onlyTransitions = true);

    const std::vector<TouchItem>& getFeaturesAtPoint(const View& _view, float _dt,
                                                     const std::vector<std::unique_ptr<Style>>& _styles,
                                                     const std::vector<std::shared_ptr<Tile>>& _tiles,
                                                     float _x, float _y, bool _visibleOnly = true);

    bool needUpdate() const { return m_needUpdate; }

protected:

    using AABB = isect2d::AABB<glm::vec2>;
    using OBB = isect2d::OBB<glm::vec2>;
    using CollisionPairs = std::vector<isect2d::ISect2D<glm::vec2>::Pair>;


    void skipTransitions(const std::vector<std::unique_ptr<Style>>& _styles,
                         const std::vector<std::shared_ptr<Tile>>& _tiles,
                         TileCache& _cache, float _currentZoom) const;

    PERF_TRACE void skipTransitions(const std::vector<const Style*>& _styles, Tile& _tile, Tile& _proxy) const;

    PERF_TRACE void sortLabels();

    PERF_TRACE void handleOcclusions(const View& _view);

    PERF_TRACE bool withinRepeatDistance(Label *_label);

    void processLabelUpdate(StyledMesh* mesh, Tile* tile,
                            const glm::mat4& mvp, const glm::vec2& screen,
                            float dt, float dz, bool drawAll,
                            bool onlyTransitions, bool isProxy);

    bool m_needUpdate;

    isect2d::ISect2D<glm::vec2> m_isect2d;

    std::vector<TouchItem> m_touchItems;

    struct LabelEntry {

        LabelEntry(Label* _label, Tile* _tile, bool _proxy)
            : label(_label),
              tile(_tile),
              priority(_label->options().priority),
              proxy(_proxy) {}

        Label* label;
        Tile* tile;
        float priority;
        bool proxy;
    };

    static bool labelComparator(const LabelEntry& _a, const LabelEntry& _b);

    std::vector<LabelEntry> m_labels;

    std::unordered_map<size_t, std::vector<Label*>> m_repeatGroups;

    float m_lastZoom;
};

}

