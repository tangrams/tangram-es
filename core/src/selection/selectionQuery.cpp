#include "selection/selectionQuery.h"

#include "gl/framebuffer.h"
#include "labels/label.h"
#include "labels/labels.h"
#include "marker/marker.h"
#include "marker/markerManager.h"
#include "tile/tileManager.h"
#include "view/view.h"

#include <cmath>

namespace Tangram {

SelectionQuery::SelectionQuery(glm::vec2 _position, float _radius, QueryCallback _queryCallback)
    : m_position(_position), m_radius(_radius), m_queryCallback(_queryCallback) {}

QueryType SelectionQuery::type() const {
    return m_queryCallback.is<FeaturePickCallback>() ? QueryType::feature :
          (m_queryCallback.is<LabelPickCallback>() ? QueryType::label : QueryType::marker);
}

void SelectionQuery::process(const View& _view, const FrameBuffer& _framebuffer, const MarkerManager& _markerManager,
                             const TileManager& _tileManager, const Labels& _labels, std::vector<SelectionColorRead>& _colorCache) const {

    float radius = m_radius * _view.pixelScale();
    glm::vec2 windowCoordinates = _view.normalizedWindowCoordinates(m_position.x - radius, m_position.y + radius);
    glm::vec2 windowSize = _view.normalizedWindowCoordinates(m_position.x + radius, m_position.y - radius) - windowCoordinates;

    GLuint color = 0;

    auto it = std::find_if(_colorCache.begin(), _colorCache.end(), [=](const auto& _colorRead) {
        return (m_position == _colorRead.position) && (m_radius == _colorRead.radius);
    });

    if (it == _colorCache.end()) {
        // Find the first non-zero color nearest to the position and within the selection radius.
        auto rect = _framebuffer.readRect(windowCoordinates.x, windowCoordinates.y, windowSize.x, windowSize.y);
        float minDistance = std::fmin(rect.width, rect.height);
        float hw = static_cast<float>(rect.width) / 2.f, hh = static_cast<float>(rect.height) / 2.f;
        for (int32_t row = 0; row < rect.height; row++) {
            for (int32_t col = 0; col < rect.width; col++) {
                uint32_t sample = rect.pixels[row * rect.width + col];
                float distance = std::hypot(row - hw, col - hh);
                if (sample != 0 && distance < minDistance) {
                    color = sample;
                    minDistance = distance;
                }
            }
        }
        // Cache the resulting color for other queries.
        _colorCache.push_back({color, m_radius, m_position});
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
        MarkerPickResult markerResult(marker->id(), {lonLat.x, lonLat.y}, {{m_position.x, m_position.y}});

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

        auto coordinate = label.second->coordToLngLat(label.first->modelCenter());

        LabelPickResult queryResult(label.first->renderType(), LngLat{coordinate.x, coordinate.y},
                                    FeaturePickResult(props, {{m_position.x, m_position.y}}));

        cb(&queryResult);
    } break;
    default: break;
    }
}
}
