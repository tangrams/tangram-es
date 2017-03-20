#include "view/flyTo.h"

#include "util/mapProjection.h"
#include "view/view.h"
#include <cmath>

namespace Tangram {

float getMinimumEnclosingZoom(double aLng, double aLat, double bLng, double bLat, const View& view, float buffer) {
    const MapProjection& projection = view.getMapProjection();
    glm::dvec2 aMeters = projection.LonLatToMeters(glm::dvec2(aLng, aLat));
    glm::dvec2 bMeters = projection.LonLatToMeters(glm::dvec2(bLng, bLat));
    double distance = glm::distance(aMeters, bMeters) * (1. + buffer);
    double focusScale = distance / (2. * MapProjection::HALF_CIRCUMFERENCE);
    double viewScale = view.getWidth() / projection.TileSize();
    double zoom = -log2(focusScale / viewScale);
    return zoom;
}

std::function<float(float)> getFlyToZoomFunction(float zStart, float zEnd, float zMax) {
    // The zoom ease function for the "fly to" animation is a quadratic function, z(t), such that:
    // z(0) = zStart,
    // z(1) = zEnd,
    // z(tMax) = zMax.
    // where tMax is the t where dz/dt = 0, i.e. the vertex of the parabola.
    // We can write z(t) as:
    // z(t) = a * t^2 + b * t + c
    // So defining z(t) is a matter of finding the appropriate coefficients 'a', 'b', and 'c'.

    // First let's define some new variables to make things easier later.
    float m = zMax - zStart;
    float f = zEnd - zStart;

    // From the first boundary condition, we get: z(0) = zStart = c.
    float c = zStart;

    // From the second boundary condition, we get: z(1) = zEnd = a + b + c,
    // which we can now reduce to: a = zEnd - zStart - b = f - b. We'll use this later.

    // To apply the third condition, we must find tMax. We differentiate z(t) and get:
    // dz/dt = 2 * a * t + b
    // At tMax, dz/dt = 0, so: 0 = 2 * a * tMax + b,
    // then: tMax = -b / (2 * a).

    // Substituting into the original expression for z(t) and simplifying, we get:
    // z(tMax) = -b^2 / (4 * a) + c = zMax.
    // Now we substitute our expressions for 'a' and 'c':
    // zMax = -b^2 / (4 * (f - b)) + zStart.
    // -b^2 / (4 * (f - b)) = zMax - zStart = m.
    // -b^2 = 4 * (f - b) * m.
    // 0 = b^2 + 4 * m * (f - b).
    // 0 = b^2 - 4 * m * b + 4 * m * f.
    // Apply the quadratic equation to find the roots for 'b' and simplify:
    // b = 2 * (m +/- sqrt(m * (m - f))).
    // For our case, we only care about the root closer to -infinity.
    float b = 2.f * (m - sqrt(m * (m - f)));

    // Now we can solve for 'a':
    float a = f - b;

    // Finally, we apply our hard-won parameters in a lambda for the ease function:
    return [=] (float t) {
        return a * t * t + b * t + c;
    };
}

std::function<float(float)> getFlyToPositionFunction(float k) {
    // Maaaaaagic!
    float a = 1.f / k;
    float b = log2((a + 1.f) / a);
    float c = -a;

    auto f = [](float x, float a, float b, float c) { return a * exp2(b * x * x * x) + c; };

    return [=] (float t) {
        if (t <= .5f) {
            return .5f * f(2.f * t, a, b, c);
        } else {
            return 1.f - .5f * f(2.f * (1.f - t), a, b, c);
        }
    };
}

} // namespace Tangram
