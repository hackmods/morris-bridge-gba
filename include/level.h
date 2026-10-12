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

constexpr int COUNT = 8;

/// Longest timber beam when a level does not set its own.
inline constexpr bn::fixed DEFAULT_BEAM_LENGTH = 60;

/// Bit flags — align with bridge::LinkRole order (track = bit 0, brace = bit 1, cable = bit 2).
inline constexpr uint8_t ALLOW_TRACK = 1;
inline constexpr uint8_t ALLOW_BRACE = 2;
inline constexpr uint8_t ALLOW_CABLE = 4;
inline constexpr uint8_t ALLOW_TRACK_AND_BRACE = ALLOW_TRACK | ALLOW_BRACE;
inline constexpr uint8_t ALLOW_TRACK_AND_CABLE = ALLOW_TRACK | ALLOW_CABLE;
inline constexpr uint8_t ALLOW_ALL = ALLOW_TRACK | ALLOW_BRACE | ALLOW_CABLE;

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
    engine::Id scheduled_engine;  ///< Timetabled engine: full stars need it or a heavier one.
    bn::fixed deck_y;       ///< Rail / start-end height for this track.
    bn::fixed safe_length;  ///< Longest timber beam the yard supplies; 0 → DEFAULT_BEAM_LENGTH.
    bn::fixed water_y;      ///< River surface; the terrain art is generated to match.
    bn::fixed parallax_back;  ///< Far layer scroll factor (fixed 8 = 1.0).
    bn::fixed parallax_front; ///< Near layer scroll factor.
    uint8_t allowed_roles;  ///< ALLOW_* bitmask for connect tools on this level.
    uint8_t intro_dialog_id; ///< 0 = none; 1..N index into intro label pairs.
    int par_cost;           ///< Target cost (joints + timber + 2× rods) for 3★; 0 = stars disabled.
    bool banked;            ///< A banking engine pushes every train from behind (Gordon's Hill).
};

[[nodiscard]] const Def& get(int level_index);

[[nodiscard]] inline bool allows_role(const Def& def, uint8_t role_bit)
{
    return (def.allowed_roles & role_bit) != 0;
}

} // namespace level

#endif
