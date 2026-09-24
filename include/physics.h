#ifndef MORRIS_BRIDGE_PHYSICS_H
#define MORRIS_BRIDGE_PHYSICS_H

/**
 * @file
 * Fixed-point structural math for Thomas's Bridge / Sodor Bridge Builder.
 *
 * ## Simplified truss stress model (GBA-friendly)
 *
 * Full FEM (direct stiffness with a dense K matrix) is too heavy for the
 * ≤15% simulation budget on GBA with no heap. Instead we use a documented
 * **load-path truss approximation** that still exposes tuneable yield limits:
 *
 * 1. Train load `W` is applied as a vertical force on the active track member,
 *    split to its endpoints by lever rule (same as classic midspan tension).
 * 2. Endpoint loads redistribute to incident members proportional to
 *    `stiffness_mul / length` (relative EA/L), so braces carry share of load.
 * 3. Axial force on member i: `|F_i|`. Cables discard compression (tension-only).
 * 4. Strain ratio: `stress = |F| / yield(material)` (stone pier yield → ∞).
 * 5. Collapse: if `stress >= 1.0`, destroy the member, immediately re-solve
 *    surrounding loads (cascade), and mark the train `DERAILED` if its path
 *    strut is gone.
 *
 * Legacy helpers (`tension`, `effective_max_tension`) remain for ratings and
 * host-test continuity; simulation prefers `stress_ratio` against yield.
 */

#include "bn_fixed.h"
#include "bn_fixed_point.h"

#include "material.h"

#include <cstdint>

namespace physics
{

/// Default car load applied to a strut the train is traversing.
inline constexpr bn::fixed BASE_WEIGHT = 4;

/// Heavier locomotive load on the engine's leading span.
inline constexpr bn::fixed ENGINE_WEIGHT = 6;

/// Length (pixels) at which a strut still has comfortable reserve strength.
inline constexpr bn::fixed SAFE_LENGTH = 48;

/// Weight of each coach when fully on a strut (load ramps with train progress).
inline constexpr bn::fixed COACH_WEIGHT = 2;

/// Peak tension a SAFE_LENGTH strut can take before it snaps (before bonuses).
inline constexpr bn::fixed BASE_MAX_TENSION = 8;

/// Extra max-tension multiplier per redundant joint beyond the first at each end.
inline constexpr bn::fixed REINFORCEMENT_PER_JOINT = bn::fixed(1) / 4;

/// Multiplier when a strut closes a triangular truss with two neighbours.
inline constexpr bn::fixed TRIANGLE_BONUS = bn::fixed(3) / 2;

/// Newton iterations for the optional reciprocal-free length refine step.
inline constexpr int SQRT_ITERS = 5;

/// Cascade re-solve passes after a snap (limits frame cost).
inline constexpr int CASCADE_MAX_PASSES = 4;

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

/**
 * @brief Normalized stress ratio in [0, ∞). Collapse when >= 1.
 */
[[nodiscard]] bn::fixed stress_ratio(bn::fixed force, bn::fixed yield_strength);

[[nodiscard]] constexpr bool exceeds_max_tension(bn::fixed current, bn::fixed max_tension)
{
    return current > max_tension;
}

[[nodiscard]] constexpr bool is_breaking(bn::fixed stress)
{
    return stress >= bn::fixed(1);
}

/**
 * @brief Discrete GBA stress heatmap band (vision palette).
 */
enum class StressBand : uint8_t
{
    green = 0,   ///< 0% – 50%
    yellow = 1,  ///< 51% – 80%
    red = 2,     ///< 81% – 99%
    breaking = 3 ///< ≥ 100% / snapped flash
};

[[nodiscard]] StressBand stress_band(bn::fixed stress);

/**
 * @brief Base span rating before structural bonuses.
 */
[[nodiscard]] bn::fixed base_max_tension_for_length(bn::fixed length,
                                                    bn::fixed safe_length = SAFE_LENGTH);

[[nodiscard]] bn::fixed max_tension_for_length(bn::fixed length,
                                               bn::fixed safe_length = SAFE_LENGTH);

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
                                              int connections_b, bool triangulated,
                                              bn::fixed safe_length = SAFE_LENGTH);

/**
 * @brief Material yield scaled by length rating (shorter spans tolerate more).
 *
 * `rated_yield = material_yield × (safe_length / length)` then × reinforcement.
 */
[[nodiscard]] bn::fixed rated_yield(material::Id material, bn::fixed length,
                                    int connections_a, int connections_b, bool triangulated,
                                    bn::fixed safe_length = SAFE_LENGTH);

/**
 * @brief Relative axial stiffness share used when redistributing joint loads.
 */
[[nodiscard]] bn::fixed member_stiffness(material::Id material, bn::fixed length);

} // namespace physics

#endif
