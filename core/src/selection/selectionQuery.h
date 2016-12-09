#pragma once

#include "util/variant.h"
#include "glm/vec2.hpp"
#include "tangram.h"
#include <map>

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

class SelectionQuery {

public:
    SelectionQuery(glm::vec2 _position, QueryCallback _queryCallback, QueryType _type);

    static void clearColorCache() { s_colorCache.clear(); }

    void process(const View& _view, const FrameBuffer& _framebuffer, const MarkerManager& _markerManager, const TileManager& _tileManager, const Labels& _labels) const;

    QueryType type() const { return m_type; }

private:
    glm::vec2 m_position;
    QueryCallback m_queryCallback;
    QueryType m_type;

    static std::map<size_t, uint32_t> s_colorCache;

};

}
