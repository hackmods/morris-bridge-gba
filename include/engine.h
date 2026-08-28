#ifndef MORRIS_BRIDGE_ENGINE_H
#define MORRIS_BRIDGE_ENGINE_H

/**
 * @file
 * Playable engines with distinct weight and speed (bridge difficulty).
 */

#include "bn_fixed.h"

namespace engine
{

enum class Id : int
{
    thomas = 0,
    percy,
    edward,
    james,
    gordon,
    count
};

struct Profile
{
    Id id;
    bn::fixed weight;
    bn::fixed speed;
    int menu_frame;
    int train_frame_base;
};

[[nodiscard]] constexpr int engine_count()
{
    return int(Id::count);
}

[[nodiscard]] const Profile& profile(Id id);

[[nodiscard]] Id default_engine();

} // namespace engine

#endif
