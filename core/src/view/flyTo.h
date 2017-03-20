#pragma once

#include <functional>

namespace Tangram {

class View;

float getMinimumEnclosingZoom(double aLng, double aLat, double bLng, double bLat, const View& view, float buffer);

std::function<float(float)> getFlyToZoomFunction(float zStart, float zEnd, float zMax);

std::function<float(float)> getFlyToPositionFunction(float k);

} // namespace Tangram
