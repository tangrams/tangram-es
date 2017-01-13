#pragma once

#include "glm/vec2.hpp"
#include "tangram.h"
#include "util/variant.h"

namespace Tangram {

class MarkerManager;
class FrameBuffer;
class TileManager;
class Labels;
class View;

enum class QueryType {
    feature,
    marker,
    label,
};

using QueryCallback = variant<FeaturePickCallback, LabelPickCallback, MarkerPickCallback>;

struct SelectionColorRead {
    uint32_t color;
    float radius;
    glm::vec2 position;
};

class SelectionQuery {

public:
    SelectionQuery(glm::vec2 _position, float _radius, QueryCallback _queryCallback);

    void process(const View& _view, const FrameBuffer& _framebuffer, const MarkerManager& _markerManager,
                 const TileManager& _tileManager, const Labels& _labels, std::vector<SelectionColorRead>& _cache) const;

    QueryType type() const;

private:
    glm::vec2 m_position;
    float m_radius;
    QueryCallback m_queryCallback;

};
}
