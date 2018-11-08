#include "view/flyTo.h"

#include "util/mapProjection.h"
#include "view/view.h"
#include <cmath>
#include "log.h"

namespace Tangram {


std::function<glm::dvec3(float)> getFlyToFunction(const View& view, glm::dvec3 start, glm::dvec3 end, double& _distance) {

    // Implementation of https://www.win.tue.nl/~vanwijk/zoompan.pdf

    // User preference for zoom/move curve sqrt(2)
    const double rho = 1.414;

    const double scale = std::pow(2.0, end.z - start.z);

    // Current view bounds in Mercator Meters
    auto rect = view.getBoundsRect();
    auto width = std::abs(rect[0][0] - rect[1][0]);
    auto height = std::abs(rect[0][1] - rect[1][1]);

    const double w0 = std::max(width, height);
    const double w1 = w0 / scale;

    const glm::dvec2 c0{start.x, start.y};
    const glm::dvec2 c1{end.x, end.y};

    const double u1 = glm::distance(c0, c1);

    auto b = [=](int i) {
                 double n = std::pow(w1, 2.0) - std::pow(w0, 2.0) +
                     (i ? -1.0 : 1.0) * std::pow(rho, 4.0) * std::pow(u1, 2.0);

                 double d = 2.0 * (i ? w1 : w0) * std::pow(rho, 2.0) * u1;
                 return n / d;
             };

    auto r = [](double b) { return std::log(-b + std::sqrt(std::pow(b, 2.0) + 1.0)); };

    // Parameterization of the elliptic path to pass through (u0,w0) and (u1,w1)
    const double r0 = r(b(0));
    const double r1 = r(b(1));
    const double S = (r1 - r0) / rho;

    _distance = std::isnan(S) ? std::abs(start.z - end.z) * 0.5 : S;

    // u, w define the elliptic path.
    auto u = [=](double s) {
                 double a = w0 / std::pow(rho, 2);
                 return a * std::cosh(r0) * std::tanh(rho * s + r0) - a * std::sinh(r0);
             };

    auto w = [=](double s) { return std::cosh(r0) / std::cosh(rho * s + r0); };

    // Check if movement is large enough to derive the fly-to curve
    bool move = u1 > std::numeric_limits<double>::epsilon();

    return [=](float t) {

                 if (t >= 1.0) {
                     return end;
                 } else if (move) {
                     double s = S * t;
                     glm::dvec2 pos = glm::mix(c0, c1, u(s) / u1);
                     double zoom = start.z - std::log2(w(s));

                     return glm::dvec3(pos.x, pos.y, zoom);
                 } else {
                     return glm::mix(start, end, t);
                 }
           };
}

} // namespace Tangram
