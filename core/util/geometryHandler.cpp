#include "util/geometryHandler.h"

GeometryHandler::GeometryHandler(const MapProjection& _mapProjection) : m_mapProjection(&_mapProjection) {
    m_tess = tessNewTess(nullptr);
}

