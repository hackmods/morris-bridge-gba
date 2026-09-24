#ifndef MORRIS_BRIDGE_MATERIAL_H
#define MORRIS_BRIDGE_MATERIAL_H

/**
 * @file
 * Sodor Bridge Builder material catalogue (vision: Wood / Iron / Stone / Cable).
 *
 * Costs are in fictional pounds (£). Yield is the axial force a member can
 * carry before stress_ratio reaches 1.0 (collapse). See docs/physics.md.
 */

#include "bn_fixed.h"

#include <cstdint>

namespace material
{

enum class Id : uint8_t
{
    wood = 0,   ///< Deck timber — light, £10, moderate yield.
    iron = 1,   ///< Girder / brace — heavy, £25, high yield.
    stone = 2,  ///< Pier — £50, infinite yield, ground-anchor only.
    cable = 3,  ///< Steel cable — £15, tension-only (zero under compression).
};

inline constexpr int COUNT = 4;

/// Legacy aliases used by older call sites / docs.
inline constexpr Id timber = Id::wood;
inline constexpr Id rod = Id::iron;

struct Def
{
    Id id;
    int cost_pounds;          ///< £ per placed member.
    bn::fixed yield_strength; ///< Axial force capacity (0 = infinite / pier).
    bn::fixed density;        ///< Self-weight share per unit length.
    bool tension_only;        ///< Cable: compression contribution is discarded.
    bool pier_only;           ///< Stone: one endpoint must be a ground anchor.
    bn::fixed stiffness_mul;  ///< Relative EA for load sharing (wood = 1).
};

inline constexpr Def TABLE[COUNT] = {
    { Id::wood, 10, bn::fixed(8), bn::fixed(1) / 4, false, false, bn::fixed(1) },
    { Id::iron, 25, bn::fixed(14), bn::fixed(1) / 2, false, false, bn::fixed(5) / 4 },
    { Id::stone, 50, bn::fixed(0), bn::fixed(1), false, true, bn::fixed(4) },
    { Id::cable, 15, bn::fixed(7), bn::fixed(1) / 16, true, false, bn::fixed(3) / 4 },
};

[[nodiscard]] inline constexpr const Def& get(Id id)
{
    return TABLE[static_cast<int>(id)];
}

[[nodiscard]] inline constexpr int cost(Id id)
{
    return get(id).cost_pounds;
}

/**
 * @brief Axial yield. Stone piers return a sentinel larger than any train load.
 */
[[nodiscard]] inline bn::fixed yield(Id id)
{
    const Def& def = get(id);

    if(def.pier_only || def.yield_strength <= 0)
    {
        return bn::fixed(1000);
    }

    return def.yield_strength;
}

[[nodiscard]] inline constexpr bool is_tension_only(Id id)
{
    return get(id).tension_only;
}

[[nodiscard]] inline constexpr bool is_pier(Id id)
{
    return get(id).pier_only;
}

[[nodiscard]] inline constexpr Id next(Id id)
{
    return static_cast<Id>((static_cast<int>(id) + 1) % COUNT);
}

[[nodiscard]] inline constexpr Id prev(Id id)
{
    return static_cast<Id>((static_cast<int>(id) + COUNT - 1) % COUNT);
}

} // namespace material

#endif
