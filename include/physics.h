#ifndef MORRIS_BRIDGE_PHYSICS_H
#define MORRIS_BRIDGE_PHYSICS_H

/**
 * @file
 * Fixed-point structural math for Thomas's Bridge.
 *
 * Tension model:
 *   Tension = Load × (DistanceFromTrain / StrutLength)
 *
 * Max tension scales with span length, joint reinforcement, and triangular
 * bracing. The locomotive applies ENGINE_WEIGHT on the first span.
 */

#include "bn_fixed.h"
#include "bn_fixed_point.h"

namespace physics
{

/// Default car load applied to a strut the train is traversing.
inline constexpr bn::fixed BASE_WEIGHT = 4;

/// Heavier locomotive load on the engine's leading span.
inline constexpr bn::fixed ENGINE_WEIGHT = 6;

/// Length (pixels) at which a strut still has comfortable reserve strength.
inline constexpr bn::fixed SAFE_LENGTH = 48;

/// Peak tension a SAFE_LENGTH strut can take before it snaps (before bonuses).
inline constexpr bn::fixed BASE_MAX_TENSION = 8;

/// Extra max-tension multiplier per redundant joint beyond the first at each end.
inline constexpr bn::fixed REINFORCEMENT_PER_JOINT = bn::fixed(1) / 4;

/// Multiplier when a strut closes a triangular truss with two neighbours.
inline constexpr bn::fixed TRIANGLE_BONUS = bn::fixed(3) / 2;

/// Newton iterations for the optional reciprocal-free length refine step.
inline constexpr int SQRT_ITERS = 5;

[[nodiscard]] constexpr bn::fixed abs(bn::fixed value)
{
    return value < 0 ? -value : value;
}

[[nodiscard]] int isqrt64(int64_t value);

[[nodiscard]] bn::fixed approx_sqrt(bn::fixed value);

[[nodiscard]] bn::fixed squared_distance(bn::fixed_point a, bn::fixed_point b);

[[nodiscard]] bn::fixed distance(bn::fixed_point a, bn::fixed_point b);

[[nodiscard]] bn::fixed_point lerp(bn::fixed_point a, bn::fixed_point b, bn::fixed t);

[[nodiscard]] bn::fixed tension(bn::fixed load, bn::fixed distance_from_train,
                                bn::fixed strut_length);

[[nodiscard]] constexpr bool exceeds_max_tension(bn::fixed current, bn::fixed max_tension)
{
    return current > max_tension;
}

/**
 * @brief Base span rating before structural bonuses.
 */
[[nodiscard]] bn::fixed base_max_tension_for_length(bn::fixed length);

/**
 * @brief Joint reinforcement from redundant connections at each endpoint.
 *
 * @param connections_a Total struts incident on node A (including this one).
 * @param connections_b Total struts incident on node B.
 */
[[nodiscard]] bn::fixed reinforcement_multiplier(int connections_a, int connections_b);

/**
 * @brief Effective max tension after length rating, joints, and triangle bracing.
 */
[[nodiscard]] bn::fixed effective_max_tension(bn::fixed length, int connections_a,
                                              int connections_b, bool triangulated);

} // namespace physics

#endif
