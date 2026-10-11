#ifndef MORRIS_BRIDGE_PHYSICS_H
#define MORRIS_BRIDGE_PHYSICS_H

/**
 * @file
 * Fixed-point geometry helpers for Thomas's Bridge.
 *
 * The structural model itself (spring truss, loads, breaking) lives in
 * `bridge::BridgeSystem`; this header only holds the float-free math it and
 * the views share.
 */

#include <stdint.h>

#include "bn_fixed.h"
#include "bn_fixed_point.h"

namespace physics
{

[[nodiscard]] constexpr bn::fixed abs(bn::fixed value)
{
    return value < 0 ? -value : value;
}

[[nodiscard]] int isqrt64(int64_t value);

[[nodiscard]] bn::fixed approx_sqrt(bn::fixed value);

[[nodiscard]] bn::fixed squared_distance(bn::fixed_point a, bn::fixed_point b);

[[nodiscard]] bn::fixed distance(bn::fixed_point a, bn::fixed_point b);

[[nodiscard]] bn::fixed_point lerp(bn::fixed_point a, bn::fixed_point b, bn::fixed t);

} // namespace physics

#endif
