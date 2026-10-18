#include "egostb/heightmap.hpp"
#include "egostb/normal_lut.hpp"
#include <cmath>
#include <algorithm>

namespace egostb {

// Evaluates interpolated map normal for clutter instance positioning using fastInvSqrtEngine
Vec3 evaluateClutterNormal(const Heightmap& hm, float x, float y) {
    return hm.peekInterpolatedNormal(x, y);
}

// Checks slope criteria for clutter object placement
bool passesClutterSlope(const Vec3& normal, float slopeFadeStart, float slopeFadeEnd, float randomVal) {
    float slopeRef = 1.0f;
    if (slopeFadeStart < slopeFadeEnd) {
        slopeRef = (normal.z - slopeFadeStart) / (slopeFadeEnd - slopeFadeStart);
        slopeRef = std::clamp(slopeRef, 0.0f, 1.0f);
    }
    return randomVal <= slopeRef;
}

} // namespace egostb
