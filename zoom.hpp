#pragma once
#include <algorithm>
#include <cmath>

namespace ZoomScroll {
// Exponential scaling feels consistent at both small and large zoom factors.
inline double nextZoom(double current, double delta, double sensitivity, double maximum) {
    if (!std::isfinite(current) || !std::isfinite(delta) || !std::isfinite(sensitivity) || !std::isfinite(maximum))
        return current;
    maximum = std::clamp(maximum, 1.0, 20.0);
    const double exponent = std::clamp(-delta * sensitivity, -20.0, 20.0);
    return std::clamp(current * std::exp(exponent), 1.0, maximum);
}
}
