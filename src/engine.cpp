#include "engine.h"

#include "physics.h"

namespace engine
{
namespace
{

constexpr Profile PROFILES[int(Id::count)] = {
    { Id::thomas, physics::BASE_WEIGHT, bn::fixed(1), 7, 0 },
    { Id::percy, bn::fixed(3), bn::fixed(1), 8, 2 },
    { Id::edward, bn::fixed(5), bn::fixed(1), 9, 4 },
    { Id::james, bn::fixed(6), bn::fixed(1), 10, 6 },
    { Id::gordon, bn::fixed(10), bn::fixed(1), 11, 8 },
};

} // namespace

const Profile& profile(const Id id)
{
    const int index = int(id);

    if(index < 0 || index >= engine_count())
    {
        return PROFILES[0];
    }

    return PROFILES[index];
}

Id default_engine()
{
    return Id::thomas;
}

} // namespace engine
