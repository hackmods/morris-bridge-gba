#ifndef MORRIS_BRIDGE_LEVEL_H
#define MORRIS_BRIDGE_LEVEL_H

/**
 * @file
 * Level definitions: seeded graph layout, art theme, and physics tuning.
 */

#include "bn_fixed.h"
#include "engine.h"

#include <cstdint>

namespace level
{

constexpr int COUNT = 3;

/// Bit flags — align with bridge::LinkRole order (track = bit 0, brace = bit 1).
inline constexpr uint8_t ALLOW_TRACK = 1;
inline constexpr uint8_t ALLOW_BRACE = 2;
inline constexpr uint8_t ALLOW_TRACK_AND_BRACE = ALLOW_TRACK | ALLOW_BRACE;

struct Seed
{
    bn::fixed x;
    bn::fixed y;
    bool anchored;
};

struct Def
{
    const Seed* seeds;
    int seed_count;
    int bg_theme_id;
    engine::Id recommended_engine;
    bn::fixed deck_y;       ///< Rail / start-end height for this track.
    bn::fixed safe_length;  ///< 0 → use physics::SAFE_LENGTH default.
    bn::fixed parallax_back;  ///< Far layer scroll factor (fixed 8 = 1.0).
    bn::fixed parallax_front; ///< Near layer scroll factor.
    uint8_t allowed_roles;  ///< ALLOW_* bitmask for connect tools on this level.
    uint8_t intro_dialog_id; ///< 0 = none; 1..N index into intro label pairs.
    int par_cost;           ///< Target nodes+links for 3★; 0 = stars disabled.
};

[[nodiscard]] const Def& get(int level_index);

[[nodiscard]] inline bool allows_role(const Def& def, uint8_t role_bit)
{
    return (def.allowed_roles & role_bit) != 0;
}

} // namespace level

#endif
