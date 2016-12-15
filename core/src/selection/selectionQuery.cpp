#include "selectionQuery.h"

#include <array>

#include "gl/framebuffer.h"
#include "labels/label.h"
#include "labels/labels.h"
#include "marker/marker.h"
#include "marker/markerManager.h"
#include "tile/tileManager.h"
#include "view/view.h"

namespace Tangram {

std::map<size_t, uint32_t> SelectionQuery::s_colorCache;

SelectionQuery::SelectionQuery(glm::vec2 _position, QueryCallback _queryCallback, QueryType _type)
    : m_position(_position), m_queryCallback(_queryCallback), m_type(_type) {}

void SelectionQuery::process(const View& _view, const FrameBuffer& _framebuffer, const MarkerManager& _markerManager,
                             const TileManager& _tileManager, const Labels& _labels) const {

    glm::vec2 windowCoordinates = _view.normalizedWindowCoordinates(m_position.x, m_position.y);

    GLuint color = 0;
    size_t seed = 0;
    hash_combine(seed, m_position.x);
    hash_combine(seed, m_position.y);

    auto it = s_colorCache.find(seed);

    if (it == s_colorCache.end()) {
        GLuint color = _framebuffer.readAt(windowCoordinates.x, windowCoordinates.y);
        s_colorCache[seed] = color;
    } else {
        color = it->second;
    }

    switch (m_type) {
    case QueryType::feature: {
        auto& cb = m_queryCallback.get<FeaturePickCallback>();

        if (color == 0) {
            cb(nullptr);
            return;
        }

        for (const auto& tile : _tileManager.getVisibleTiles()) {
            if (auto props = tile->getSelectionFeature(color)) {
                FeaturePickResult queryResult(props, {{m_position.x, m_position.y}});
                cb(&queryResult);
                return;
            }
        }

        cb(nullptr);
    } break;
    case QueryType::marker: {
        auto& cb = m_queryCallback.get<MarkerPickCallback>();

        if (color == 0) {
            cb(nullptr);
            return;
        }

        auto marker = _markerManager.getMarkerOrNullBySelectionColor(color);

        if (!marker) {
            cb(nullptr);
            return;
        }

        MarkerPickResult markerResult(marker->id(), {{m_position.x, m_position.y}});

        cb(&markerResult);
    } break;
    case QueryType::label: {
        auto& cb = m_queryCallback.get<LabelPickCallback>();

        if (color == 0) {
            cb(nullptr);
            return;
        }

        auto label = _labels.getLabel(color);

        if (!label.first) {
            cb(nullptr);
            return;
        }

        auto props = label.second->getSelectionFeature(label.first->options().featureId);

        if (!props) {
            cb(nullptr);
            return;
        }

        LngLat coordinates = label.first->coordinates(*label.second, _view.getMapProjection());

        LabelPickResult queryResult(label.first->renderType(), coordinates,
                                    FeaturePickResult(props, {{m_position.x, m_position.y}}));

        cb(&queryResult);
    } break;
    default: break;
    }
}
}
