#include "zoom.hpp"
#include <cassert>
#include <limits>

int main() {
    using ZoomScroll::nextZoom;
    assert(nextZoom(1, -10, .01, 8) > 1);
    assert(nextZoom(2, 10, .01, 8) < 2);
    assert(nextZoom(1, 1000, .01, 8) == 1);
    assert(nextZoom(8, -1000, .01, 8) == 8);
    assert(nextZoom(2, 0, .01, 8) == 2);
    assert(nextZoom(2, -10, -.01, 8) < 2);
    assert(nextZoom(2, 10, 0, 8) == 2);
    assert(std::abs(nextZoom(nextZoom(2, -10, .01, 8), 10, .01, 8) - 2) < 1e-12);
    assert(nextZoom(2, std::numeric_limits<double>::quiet_NaN(), .01, 8) == 2);
    assert(nextZoom(1, -1e300, .1, 8) == 8);
}
