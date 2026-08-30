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

constexpr Seed Tidmouth[] = {
    { -72, game::TRACK_Y, true },
    { 72, game::TRACK_Y, true },
    { -88, game::TRACK_Y, true },
    { -56, game::TRACK_Y, true },
    { 56, game::TRACK_Y, true },
    { 88, game::TRACK_Y, true },
};

constexpr Seed Wellsworth[] = {
    { -100, 48, true },
    { 100, 48, true },
    { -116, game::TRACK_Y, true },
    { -84, game::TRACK_Y, true },
    { 84, game::TRACK_Y, true },
    { 116, game::TRACK_Y, true },
};

constexpr Seed CrovanGate[] = {
    { -96, 36, true },
    { 96, 36, true },
    { -112, game::TRACK_Y, true },
    { -80, game::TRACK_Y, true },
    { 80, game::TRACK_Y, true },
    { 112, game::TRACK_Y, true },
};

constexpr Seed CallanCanal[] = {
    { -108, 50, true },
    { 108, 50, true },
    { -120, game::TRACK_Y, true },
    { -92, game::TRACK_Y, true },
    { 92, game::TRACK_Y, true },
    { 120, game::TRACK_Y, true },
};

constexpr Seed SodorSummit[] = {
    { -112, 40, true },
    { 112, 40, true },
    { -120, game::TRACK_Y, true },
    { -96, game::TRACK_Y, true },
    { 96, game::TRACK_Y, true },
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
        1,
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
        2,
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
        3,
        12,
    },
    {
        Tidmouth,
        int(sizeof(Tidmouth) / sizeof(Seed)),
        0,
        engine::Id::percy,
        game::TRACK_Y,
        physics::SAFE_LENGTH,
        bn::fixed(1) / 4,
        bn::fixed(1) / 2,
        ALLOW_TRACK,
        4,
        5,
    },
    {
        Wellsworth,
        int(sizeof(Wellsworth) / sizeof(Seed)),
        1,
        engine::Id::thomas,
        bn::fixed(48),
        bn::fixed(44),
        bn::fixed(1) / 3,
        bn::fixed(3) / 5,
        ALLOW_TRACK_AND_BRACE,
        5,
        10,
    },
    {
        CrovanGate,
        int(sizeof(CrovanGate) / sizeof(Seed)),
        2,
        engine::Id::edward,
        bn::fixed(36),
        bn::fixed(40),
        bn::fixed(1) / 4,
        bn::fixed(3) / 5,
        ALLOW_TRACK_AND_BRACE,
        6,
        12,
    },
    {
        CallanCanal,
        int(sizeof(CallanCanal) / sizeof(Seed)),
        0,
        engine::Id::james,
        bn::fixed(50),
        bn::fixed(36),
        bn::fixed(1) / 3,
        bn::fixed(2) / 3,
        ALLOW_TRACK_AND_BRACE,
        7,
        14,
    },
    {
        SodorSummit,
        int(sizeof(SodorSummit) / sizeof(Seed)),
        1,
        engine::Id::james,
        bn::fixed(40),
        bn::fixed(36),
        bn::fixed(1) / 3,
        bn::fixed(2) / 3,
        ALLOW_TRACK_AND_BRACE,
        8,
        16,
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
