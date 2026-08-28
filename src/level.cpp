#include "level.h"

#include "game_constants.h"
#include "physics.h"

namespace level
{
namespace
{

constexpr Seed Knapford[] = {
    { -88, game::TRACK_Y, true },
    { 88, game::TRACK_Y, true },
    { -104, game::TRACK_Y, true },
    { -72, game::TRACK_Y, true },
    { 72, game::TRACK_Y, true },
    { 104, game::TRACK_Y, true },
};

constexpr Seed GordonHill[] = {
    { -96, 44, true },
    { 96, 44, true },
    { -112, game::TRACK_Y, true },
    { -80, game::TRACK_Y, true },
    { 80, game::TRACK_Y, true },
    { 112, game::TRACK_Y, true },
};

constexpr Seed Vicarstown[] = {
    { -104, 48, true },
    { 104, 48, true },
    { -120, game::TRACK_Y, true },
    { -88, game::TRACK_Y, true },
    { 88, game::TRACK_Y, true },
    { 120, game::TRACK_Y, true },
};

constexpr Def DEFINITIONS[COUNT] = {
    {
        Knapford,
        int(sizeof(Knapford) / sizeof(Seed)),
        0,
        engine::Id::thomas,
        game::TRACK_Y,
        physics::SAFE_LENGTH,
        bn::fixed(1) / 4,
        bn::fixed(1) / 2,
        ALLOW_TRACK,
        5,
    },
    {
        GordonHill,
        int(sizeof(GordonHill) / sizeof(Seed)),
        1,
        engine::Id::edward,
        bn::fixed(44),
        bn::fixed(44),
        bn::fixed(1) / 3,
        bn::fixed(3) / 5,
        ALLOW_TRACK_AND_BRACE,
        11,
    },
    {
        Vicarstown,
        int(sizeof(Vicarstown) / sizeof(Seed)),
        2,
        engine::Id::james,
        bn::fixed(48),
        bn::fixed(40),
        bn::fixed(1) / 3,
        bn::fixed(2) / 3,
        ALLOW_TRACK_AND_BRACE,
        12,
    },
};

} // namespace

const Def& get(const int level_index)
{
    if(level_index < 0 || level_index >= COUNT)
    {
        return DEFINITIONS[0];
    }

    return DEFINITIONS[level_index];
}

} // namespace level
