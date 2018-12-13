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

namespace Tangram {

class FontContext;
class LabelSet;
class Marker;
class Tile;
class Style;
class Scene;
class TileManager;

class LabelManager {

public:
    LabelManager();

    virtual ~LabelManager();

    void drawDebug(RenderState& rs, const View& _view);

    void updateLabelSet(const ViewState& _viewState, float _dt, const Scene& _scene,
                        const std::vector<std::shared_ptr<Tile>>& _tiles,
                        const std::vector<std::unique_ptr<Marker>>& _markers,
                        TileManager& tileManager);

    /* onlyRender: when the view and tiles have not changed one does not need to update the set of
     * active labels. We just need to render these the labels in this case
     */
    void updateLabels(const ViewState& _viewState, float _dt,
                      const std::vector<std::unique_ptr<Style>>& _styles,
                      const std::vector<std::shared_ptr<Tile>>& _tiles,
                      const std::vector<std::unique_ptr<Marker>>& _markers,
                      bool _onlyRender = true);

    bool needUpdate() const { return m_needUpdate; }

    std::pair<Label*, const Tile*> getLabel(uint32_t _selectionColor) const;

protected:

    using AABB = isect2d::AABB<glm::vec2>;
    using OBB = isect2d::OBB<glm::vec2>;
    using CollisionPairs = std::vector<isect2d::ISect2D<glm::vec2>::Pair>;


    void skipTransitions(const Scene& _scene, const std::vector<std::shared_ptr<Tile>>& _tiles,
                         TileManager& _tileManager, float _currentZoom) const;

    void skipTransitions(const std::vector<const Style*>& _styles, Tile& _tile, Tile& _proxy) const;

    void handleOcclusions(const ViewState& _viewState);

    bool withinRepeatDistance(Label *_label);

    void processLabelUpdate(const ViewState& _viewState, const LabelSet* _labelSet, Style* _style,
                            const Tile* _tile, const Marker *_marker, const glm::mat4& _mvp,
                            float _dt, bool _drawAll, bool _onlyRender, bool _isProxy);

    bool m_needUpdate;

    isect2d::ISect2D<glm::vec2> m_isect2d;

    struct LabelEntry {

        LabelEntry(Label* _label, Style* _style, const Tile* _tile, const Marker* _marker,
                   bool _proxy, Range _screenTransform)
            : label(_label),
              style(_style),
              tile(_tile),
              marker(_marker),
              priority(_label->options().priority),
              proxy(_proxy),
              transformRange(_screenTransform) {}

        Label* label;
        Style* style;
        const Tile* tile;
        const Marker* marker;
        float priority;
        bool proxy;

        Range transformRange;
        Range obbsRange;
    };

    static bool priorityComparator(const LabelEntry& _a, const LabelEntry& _b);

    static bool zOrderComparator(const LabelEntry& _a, const LabelEntry& _b);

    std::vector<OBB> m_obbs;
    ScreenTransform::Buffer m_transforms;

    std::vector<LabelEntry> m_labels;
    std::vector<LabelEntry> m_selectionLabels;

    std::unordered_map<size_t, std::vector<Label*>> m_repeatGroups;

    float m_lastZoom;
};

}

