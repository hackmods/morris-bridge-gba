#include "engine.h"

#include "physics.h"

namespace engine
{
namespace
{

constexpr Profile PROFILES[int(Id::count)] = {
    { physics::BASE_WEIGHT, bn::fixed(1) }, // Thomas
    { bn::fixed(3), bn::fixed(1) },         // Percy — light reward engine
    { bn::fixed(5), bn::fixed(1) },         // Edward
    { bn::fixed(6), bn::fixed(1) },         // James
    { bn::fixed(10), bn::fixed(1) },        // Gordon — prestige hard mode
};

} // namespace

const Profile& profile(const Id id)
{
    const int index = int(id);

    if(! is_playable(id))
    {
        return PROFILES[int(default_engine())];
    }

    return PROFILES[index];
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
            return Id::james;

        default:
            return none;
    }
}

Id all_tracks_bonus_engine()
{
    return Id::gordon;
}

bool is_playable(const Id id)
{
    return id >= Id::thomas && id < Id::count;
}

} // namespace engine
