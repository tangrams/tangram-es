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

SelectionQuery::SelectionQuery(glm::vec2 _position, QueryCallback _queryCallback)
    : m_position(_position), m_queryCallback(_queryCallback) {}

QueryType SelectionQuery::type() const {
    return m_queryCallback.is<FeaturePickCallback>() ? QueryType::feature :
          (m_queryCallback.is<LabelPickCallback>() ? QueryType::label : QueryType::marker);
}

void SelectionQuery::process(const View& _view, const FrameBuffer& _framebuffer, const MarkerManager& _markerManager,
                             const TileManager& _tileManager, const Labels& _labels, std::vector<SelectionColorRead>& _colorCache) const {

    glm::vec2 windowCoordinates = _view.normalizedWindowCoordinates(m_position.x, m_position.y);

    GLuint color = 0;

    auto it = std::find_if(_colorCache.begin(), _colorCache.end(), [=](const auto& _colorRead) {
        return m_position == _colorRead.position;
    });

    if (it == _colorCache.end()) {
        color = _framebuffer.readAt(windowCoordinates.x, windowCoordinates.y);
        _colorCache.push_back({color, m_position});
    } else {
        color = it->color;
    }

    switch (type()) {
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

        glm::dvec2 bbCenter = marker->bounds().center();
        glm::dvec2 lonLat = _view.getMapProjection().MetersToLonLat(bbCenter);
        bool clipped;
        glm::vec2 screenPosition = _view.lonLatToScreenPosition(lonLat.x, lonLat.y, clipped);

        if (clipped) {
            cb(nullptr);
            return;
        }

        MarkerPickResult markerResult(marker->id(), {{screenPosition.x, screenPosition.y}});

        cb(&markerResult);
    } break;
    case QueryType::label: {
        auto& cb = m_queryCallback.get<LabelPickCallback>();

        if (color == 0) {
            cb(nullptr);
            return;
        }

        auto label = _labels.getLabel(color);

        if (!label.first || !label.second) {
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
