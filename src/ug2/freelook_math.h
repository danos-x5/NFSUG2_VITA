#ifndef UG2_FREELOOK_MATH_H_
#define UG2_FREELOOK_MATH_H_

// Free look maths: stick direction to view yaw, the look-back half, and turning the chase camera rig like Look Back.

#include <cmath>

namespace ug2 { namespace freelook_math {

const float DeadZone = 0.30f;           // radial, of full deflection
const float SnapDegrees = 8.0f;         // stick this close to straight up or down reads exactly 0 or 180
const float BackHysteresis = 5.0f;      // degrees past 90 before the look-back half changes

struct Vec
{
    float x, y, z;
};

// Stick direction as a view yaw.
struct Aim
{
    bool  held;     // outside the dead zone
    float yaw;      // degrees clockwise from ahead: 90 right, 180 behind, -90 left
};

// Degrees into -180..180; 180 stays 180.
inline float wrap(float degrees)
{
    float r = std::fmod(degrees, 360.0f);
    if (r > 180.0f)
        r -= 360.0f;
    else if (r <= -180.0f)
        r += 360.0f;
    return r;
}

// Stick x right, y down, -1..1.
inline Aim aim(float x, float y, float deadZone = DeadZone)
{
    if (!(std::hypot(x, y) > deadZone))
        return { false, 0.0f };
    float yaw = float(std::atan2(double(x), double(-y)) * (180.0 / 3.14159265358979323846));
    if (std::fabs(yaw) <= SnapDegrees)
        yaw = 0.0f;
    else if (std::fabs(yaw) >= 180.0f - SnapDegrees)
        yaw = 180.0f;
    return { true, yaw };
}

// Look-back half for this yaw, with hysteresis at 90 degrees.
inline bool backHalf(float yaw, bool wasBack)
{
    const float edge = 90.0f + (wasBack ? -BackHysteresis : BackHysteresis);
    return std::fabs(yaw) > edge;
}

// Yaw still to turn after the game's own mirror.
inline float residual(float yaw, bool mirrored)
{
    return mirrored ? wrap(yaw - 180.0f) : wrap(yaw);
}

inline float dot(const Vec& a, const Vec& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec cross(const Vec& a, const Vec& b)
{
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}
inline Vec row(const float* m, int r) { return { m[4 * r], m[4 * r + 1], m[4 * r + 2] }; }

// Turns p about m's car-space z axis, x toward y.
inline Vec spin(const float* m, const Vec& p, float cosA, float sinA)
{
    const Vec r0 = row(m, 0), r1 = row(m, 1), r2 = row(m, 2), o = row(m, 3);
    const Vec d = { p.x - o.x, p.y - o.y, p.z - o.z };
    const float lx = dot(d, r0) / dot(r0, r0), ly = dot(d, r1) / dot(r1, r1), lz = dot(d, r2) / dot(r2, r2);
    const float nx = lx * cosA - ly * sinA, ny = lx * sinA + ly * cosA;
    return { o.x + nx * r0.x + ny * r1.x + lz * r2.x, o.y + nx * r0.y + ny * r1.y + lz * r2.y,
             o.z + nx * r0.z + ny * r1.z + lz * r2.z };
}

// +1 when an x-toward-y turn moves the view right.
inline float rightTurnSign(const float* m, const Vec& forward, const Vec& up)
{
    const Vec r0 = row(m, 0), r1 = row(m, 1);
    const float fx = dot(forward, r0) / dot(r0, r0), fy = dot(forward, r1) / dot(r1, r1);
    const Vec turn = { -fy * r0.x + fx * r1.x, -fy * r0.y + fx * r1.y, -fy * r0.z + fx * r1.z };
    return dot(turn, cross(forward, up)) >= 0.0f ? 1.0f : -1.0f;
}

// Turn of yaw degrees right; exact at 0 and 180.
inline void turn(float yaw, float sign, float* cosA, float* sinA)
{
    if (yaw == 0.0f)
    {
        *cosA = 1.0f;
        *sinA = 0.0f;
        return;
    }
    if (std::fabs(yaw) == 180.0f)
    {
        *cosA = -1.0f;
        *sinA = 0.0f;
        return;
    }
    const double a = double(yaw) * (3.14159265358979323846 / 180.0);
    *cosA = float(std::cos(a));
    *sinA = float(std::sin(a)) * sign;
}

}}

#endif /* !UG2_FREELOOK_MATH_H_ */
