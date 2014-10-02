#pragma once

#include <vector>

#include "json/json.h"
#include "tesselator.h"
#include "glm/glm.hpp"

#include "util/projection.h"
#include "platform.h"


class GeometryHandler {
    TESStesselator* m_tess;
    MapProjection *m_mapProjection;

public:
    GeometryHandler(MapProjection* m_mapProjection);

    template<typename T>
    void polygonAddData(Json::Value _geomCoordinates, std::vector<T>& _vertices, std::vector<GLushort>& _indices, glm::vec4& _rgba, glm::dvec2 _tileOffset, double _height, double _minHeight);

    virtual ~GeometryHandler() {
        tessDeleteTess(m_tess);
    }
};
