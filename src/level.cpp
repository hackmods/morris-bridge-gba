#include "level.h"

#include "physics.h"

namespace level
{
namespace
{

constexpr Seed Knapford[] = {
    { -88, 16, true },
    { 88, 16, true },
    { -104, 40, true },
    { -88, 40, true },
    { -72, 40, true },
    { 72, 40, true },
    { 88, 40, true },
    { 104, 40, true },
};

constexpr Seed GordonHill[] = {
    { -96, 8, true },
    { 96, 8, true },
    { -112, 32, true },
    { -80, 32, true },
    { 80, 32, true },
    { 112, 32, true },
};

constexpr Seed Vicarstown[] = {
    { -104, 12, true },
    { 104, 12, true },
    { -120, 36, true },
    { -88, 36, true },
    { 88, 36, true },
    { 120, 36, true },
};

constexpr Def DEFINITIONS[COUNT] = {
    {
        Knapford,
        int(sizeof(Knapford) / sizeof(Seed)),
        0,
        engine::Id::thomas,
        physics::SAFE_LENGTH,
        bn::fixed(1) / 4,
        bn::fixed(1) / 2,
    },
    {
        GordonHill,
        int(sizeof(GordonHill) / sizeof(Seed)),
        1,
        engine::Id::edward,
        bn::fixed(44),
        bn::fixed(1) / 3,
        bn::fixed(3) / 5,
    },
    {
        Vicarstown,
        int(sizeof(Vicarstown) / sizeof(Seed)),
        2,
        engine::Id::james,
        bn::fixed(40),
        bn::fixed(1) / 3,
        bn::fixed(2) / 3,
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
