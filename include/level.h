#ifndef MORRIS_BRIDGE_LEVEL_H
#define MORRIS_BRIDGE_LEVEL_H

/**
 * @file
 * Level definitions: seeded graph layout, art theme, and physics tuning.
 */

#include "bn_fixed.h"
#include "engine.h"

namespace level
{

constexpr int COUNT = 3;

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
    bn::fixed safe_length; ///< 0 → use physics::SAFE_LENGTH default.
    bn::fixed parallax_back;  ///< Far layer scroll factor (fixed 8 = 1.0).
    bn::fixed parallax_front; ///< Near layer scroll factor.
};

[[nodiscard]] const Def& get(int level_index);

} // namespace level

#endif
