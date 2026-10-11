#include "engine.h"

namespace engine
{
namespace
{

constexpr Car NO_CAR = { CarKind::coach, 0 };

// Indexed by Id. Weight is the engine alone; the cars add theirs.
constexpr Profile PROFILES[int(Id::count)] = {
    // Thomas and his two coaches.
    { 4, 1, 2, { { CarKind::annie, 1 }, { CarKind::clarabel, 1 }, NO_CAR } },
    // Percy with a truck off the yard.
    { 3, 1, 1, { { CarKind::truck, 1 }, NO_CAR, NO_CAR } },
    // Edward: old and gentle.
    { 4, bn::fixed(3) / 4, 2, { { CarKind::tender_blue, 2 }, { CarKind::coach, 1 }, NO_CAR } },
    // James, who would rather not pull trucks.
    { 5, bn::fixed(5) / 4, 2, { { CarKind::tender_red, 2 }, { CarKind::coach, 1 }, NO_CAR } },
    // Gordon and the express: it does not slow down.
    { 8, bn::fixed(3) / 2, 3,
      { { CarKind::tender_blue, 2 }, { CarKind::coach, 2 }, { CarKind::coach, 2 } } },
    // Henry with the Flying Kipper.
    { 5, 1, 3, { { CarKind::tender_green, 2 }, { CarKind::truck, 1 }, { CarKind::truck, 1 } } },
};

bool is_playable_impl(const Id id)
{
    return id >= Id::thomas && id < Id::count;
}

} // namespace

const Profile& profile(const Id id)
{
    if(! is_playable_impl(id))
    {
        return PROFILES[int(default_engine())];
    }

    return PROFILES[int(id)];
}

Id default_engine()
{
    return Id::thomas;
}

Id unlock_for_level_clear(const int level_index)
{
    switch(level_index)
    {
        case 0:
            return Id::percy;
        case 1:
            return Id::edward;
        case 2:
            return Id::henry;
        case 3:
            return Id::james;
        default:
            return none;
    }
}

Id all_tracks_bonus_engine()
{
    return Id::gordon;
}

bn::fixed total_weight(const Id id)
{
    const Profile& entry = profile(id);
    bn::fixed total = entry.weight;

    for(int i = 0; i < entry.car_count; ++i)
    {
        total += entry.cars[i].weight;
    }

    return total;
}

int roster_index(const Id id)
{
    for(int i = 0; i < int(Id::count); ++i)
    {
        if(ROSTER[i] == id)
        {
            return i;
        }
    }

    return 0;
}

bool is_playable(const Id id)
{
    return is_playable_impl(id);
}

} // namespace engine
