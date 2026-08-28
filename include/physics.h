#ifndef RYANS_BRIDGE_PHYSICS_H
#define RYANS_BRIDGE_PHYSICS_H

/**
 * @file
 * Fixed-point structural math for Ryan's Bridge.
 *
 * Every quantity is a Butano `bn::fixed` (20.12). There is no `float` or
 * `double` in this module: distances, lengths, and tension are computed with
 * integer shifts and 64-bit intermediates.
 *
 * Tension model (as specified):
 *   Tension = BaseWeight × (DistanceFromTrain / StrutLength)
 *
 * `DistanceFromTrain` is the distance the train has travelled along the
 * current strut, in the same units as `StrutLength`. When the train is at
 * the far node, the ratio is 1 and tension equals BaseWeight.
 */

#include "bn_fixed.h"
#include "bn_fixed_point.h"

namespace physics
{

/// Default train load applied to the strut it is traversing.
inline constexpr bn::fixed BASE_WEIGHT = 4;

/// Length (pixels) at which a strut still has comfortable reserve strength.
inline constexpr bn::fixed SAFE_LENGTH = 48;

/// Peak tension a SAFE_LENGTH strut can take before it snaps.
inline constexpr bn::fixed BASE_MAX_TENSION = 8;

/// Newton iterations for the optional reciprocal-free length refine step.
inline constexpr int SQRT_ITERS = 5;

/**
 * @brief Absolute value of a 20.12 fixed-point number.
 */
[[nodiscard]] constexpr bn::fixed abs(bn::fixed value)
{
    return value < 0 ? -value : value;
}

/**
 * @brief Integer square root of a non-negative 64-bit value.
 *
 * Binary restoring algorithm — no libm, no BIOS `Sqrt` SWI. Used so host
 * unit tests and the GBA ROM share the same length approximation.
 */
[[nodiscard]] int isqrt64(int64_t value);

/**
 * @brief Fast square root of a non-negative `bn::fixed`.
 *
 * Computes `sqrt(x)` as `isqrt(x.data() << 12) / 4096`, which is exact for
 * perfect squares representable in 20.12 (for example 3-4-5).
 */
[[nodiscard]] bn::fixed approx_sqrt(bn::fixed value);

/**
 * @brief Squared Euclidean distance. Prefer this for radius tests.
 *
 * Comparing `squared_distance` against `radius * radius` avoids a sqrt in
 * the cursor-snap hot path.
 */
[[nodiscard]] bn::fixed squared_distance(bn::fixed_point a, bn::fixed_point b);

/**
 * @brief Euclidean distance between two nodes, via `approx_sqrt`.
 */
[[nodiscard]] bn::fixed distance(bn::fixed_point a, bn::fixed_point b);

/**
 * @brief Linear interpolation of two points. `t` is 0 at `a` and 1 at `b`.
 */
[[nodiscard]] bn::fixed_point lerp(bn::fixed_point a, bn::fixed_point b, bn::fixed t);

/**
 * @brief Instantaneous strut tension.
 *
 * @param base_weight Typical train load (`BASE_WEIGHT`).
 * @param distance_from_train How far the train has moved along the strut.
 * @param strut_length Rest length stored when the strut was placed.
 * @return 0 if `strut_length` is 0 (degenerate beam).
 */
[[nodiscard]] bn::fixed tension(bn::fixed base_weight, bn::fixed distance_from_train,
                                bn::fixed strut_length);

/**
 * @brief True when `current` is strictly greater than `max_tension`.
 */
[[nodiscard]] constexpr bool exceeds_max_tension(bn::fixed current, bn::fixed max_tension)
{
    return current > max_tension;
}

/**
 * @brief Max tension for a newly placed strut: longer spans are weaker.
 *
 * `max = BASE_MAX_TENSION * SAFE_LENGTH / length`, clamped to a minimum of
 * 1 so a ridiculous span still has a defined limit.
 */
[[nodiscard]] bn::fixed max_tension_for_length(bn::fixed length);

} // namespace physics

#endif
