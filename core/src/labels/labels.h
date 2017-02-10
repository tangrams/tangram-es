#pragma once

#include "data/properties.h"
#include "labels/label.h"
#include "labels/screenTransform.h"
#include "labels/spriteLabel.h"
#include "tile/tileID.h"

#include "glm_vec.h" // for isect2d.h
#include "isect2d.h"

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>
#include <vector>

#define PERF_TRACE __attribute__ ((noinline))

namespace Tangram {

class FontContext;
class Marker;
class Tile;
class Style;
class TileCache;

class Labels {

public:
    Labels();

    virtual ~Labels();

    void drawDebug(RenderState& rs, const View& _view);

    void updateLabelSet(const ViewState& _viewState, float _dt,
                        const std::vector<std::unique_ptr<Style>>& _styles,
                        const std::vector<std::shared_ptr<Tile>>& _tiles,
                        const std::vector<std::unique_ptr<Marker>>& _markers,
                        TileCache& _cache);

    PERF_TRACE void updateLabels(const ViewState& _viewState, float _dt,
                                 const std::vector<std::unique_ptr<Style>>& _styles,
                                 const std::vector<std::shared_ptr<Tile>>& _tiles,
                                 const std::vector<std::unique_ptr<Marker>>& _markers,
                                 bool _onlyTransitions = true);

    bool needUpdate() const { return m_needUpdate; }

    std::pair<Label*, Tile*> getLabel(uint32_t _selectionColor) const;

protected:

    using AABB = isect2d::AABB<glm::vec2>;
    using OBB = isect2d::OBB<glm::vec2>;
    using CollisionPairs = std::vector<isect2d::ISect2D<glm::vec2>::Pair>;


    void skipTransitions(const std::vector<std::unique_ptr<Style>>& _styles,
                         const std::vector<std::shared_ptr<Tile>>& _tiles,
                         TileCache& _cache, float _currentZoom) const;

    PERF_TRACE void skipTransitions(const std::vector<const Style*>& _styles, Tile& _tile, Tile& _proxy) const;

    PERF_TRACE void sortLabels();

    PERF_TRACE void handleOcclusions(const ViewState& _viewState);

    PERF_TRACE bool withinRepeatDistance(Label *_label);

    void processLabelUpdate(const ViewState& viewState, StyledMesh* mesh, Tile* tile,
                            const glm::mat4& mvp, float dt, bool drawAll,
                            bool onlyTransitions, bool isProxy);

    bool m_needUpdate;

    isect2d::ISect2D<glm::vec2> m_isect2d;

    struct LabelEntry {

        LabelEntry(Label* _label, Tile* _tile, bool _proxy, Range _screenTransform)
            : label(_label),
              tile(_tile),
              priority(_label->options().priority),
              proxy(_proxy),
              transformRange(_screenTransform) {}

        Label* label;
        Tile* tile;
        float priority;
        bool proxy;

        Range transformRange;
        Range obbsRange;
    };

    static bool labelComparator(const LabelEntry& _a, const LabelEntry& _b);

    std::vector<OBB> m_obbs;
    ScreenTransform::Buffer m_transforms;

    std::vector<LabelEntry> m_labels;
    std::vector<LabelEntry> m_selectionLabels;

    std::unordered_map<size_t, std::vector<Label*>> m_repeatGroups;

    float m_lastZoom;
};

}

