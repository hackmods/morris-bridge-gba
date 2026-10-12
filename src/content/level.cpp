#include "level.h"

namespace level
{
namespace
{

// Seed 0 is the start bank, seed 1 the end bank; the rest are the only other
// solid ground a level offers. Every span between them has to be engineered.
// Nothing sits lower than y = 40: the HUD owns the bottom of the screen.

// Tutorial: one riverbed rock under mid-span. Prop the deck or lose it.
constexpr Seed Knapford[] = {
    { -40, 12, true },
    { 40, 12, true },
    { 0, 40, true },
};

// The climb: the far bank is 32 px higher, two crags overhang the line, and
// Edward banks every train from behind.
constexpr Seed GordonHill[] = {
    { -80, 44, true },
    { 80, 12, true },
    { -36, -24, true },
    { 36, -36, true },
};

// Nothing but the two sides of the ravine: the span must be a truss.
constexpr Seed CuldeeFell[] = {
    { -60, 30, true },
    { 60, 30, true },
};

// Timber only, no rock: a queen-post truss in wood.
constexpr Seed Tidmouth[] = {
    { -48, 30, true },
    { 48, 30, true },
};

// Deep cutting with ledges low on each wall: strut the deck from below.
constexpr Seed Wellsworth[] = {
    { -56, 0, true },
    { 56, 0, true },
    { -56, 40, true },
    { 56, 40, true },
};

// One off-centre rock in a wide river: a trestle plus a long free span.
constexpr Seed CrovanGate[] = {
    { -88, 0, true },
    { 88, 0, true },
    { -16, 40, true },
};

// Low line between the two towers of Callan Castle: stays from above.
constexpr Seed Callan[] = {
    { -72, 40, true },
    { 72, 40, true },
    { -72, -20, true },
    { 72, -20, true },
};

// The bridge to the mainland: the longest span, one pier in the channel.
constexpr Seed Vicarstown[] = {
    { -96, 8, true },
    { 96, 8, true },
    { 0, 40, true },
};

#define SEEDS(name) name, int(sizeof(name) / sizeof(Seed))

// seeds, theme, scheduled engine, deck y, beam length, river y, parallax
// back/front, tools, intro, par, banked. tools/gen_graphics.py reads this table (and
// the seeds above) to draw each level's terrain, so keep the layout regular.
constexpr Def DEFINITIONS[COUNT] = {
    { SEEDS(Knapford), 0, engine::Id::thomas, 12, 60, 54,
      bn::fixed(1) / 4, bn::fixed(1) / 2, ALLOW_TRACK, 1, 4, false },
    { SEEDS(GordonHill), 1, engine::Id::thomas, 44, 60, 60,
      bn::fixed(1) / 3, bn::fixed(3) / 5, ALLOW_TRACK_AND_CABLE, 2, 10, true },
    { SEEDS(CuldeeFell), 1, engine::Id::edward, 30, 56, 54,
      bn::fixed(1) / 3, bn::fixed(2) / 3, ALLOW_ALL, 3, 20, false },
    { SEEDS(Tidmouth), 0, engine::Id::percy, 30, 56, 54,
      bn::fixed(1) / 4, bn::fixed(1) / 2, ALLOW_TRACK, 4, 13, false },
    { SEEDS(Wellsworth), 1, engine::Id::henry, 0, 60, 58,
      bn::fixed(1) / 3, bn::fixed(3) / 5, ALLOW_ALL, 5, 9, false },
    { SEEDS(CrovanGate), 0, engine::Id::edward, 0, 60, 54,
      bn::fixed(1) / 4, bn::fixed(3) / 5, ALLOW_ALL, 6, 20, false },
    { SEEDS(Callan), 0, engine::Id::james, 40, 60, 60,
      bn::fixed(1) / 3, bn::fixed(2) / 3, ALLOW_ALL, 7, 15, false },
    { SEEDS(Vicarstown), 2, engine::Id::henry, 8, 60, 54,
      bn::fixed(1) / 3, bn::fixed(2) / 3, ALLOW_ALL, 8, 25, false },
};

#undef SEEDS

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
