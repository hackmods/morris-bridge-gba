#ifndef MORRIS_BRIDGE_ENGINE_H
#define MORRIS_BRIDGE_ENGINE_H

/**
 * @file
 * Playable engines with distinct weight and speed (bridge difficulty).
 *
 * Sprite/menu indices are derived from Id — do not duplicate in data tables.
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

/// Invalid / no-engine sentinel (not a playable Id).
inline constexpr Id none = Id::count;

inline constexpr int FRAMES_PER_ENGINE = 2;
inline constexpr int MENU_ENGINE_BASE = 7;

struct Profile
{
    bn::fixed weight;
    bn::fixed speed;
    int coach_count;
};

[[nodiscard]] constexpr int engine_count()
{
    return int(Id::count);
}

[[nodiscard]] constexpr int menu_frame(const Id id)
{
    return MENU_ENGINE_BASE + int(id);
}

[[nodiscard]] constexpr int train_frame_base(const Id id)
{
    return int(id) * FRAMES_PER_ENGINE;
}

[[nodiscard]] const Profile& profile(Id id);

[[nodiscard]] Id default_engine();

/** Engine unlocked on first clear of @p level_index, or `none`. */
[[nodiscard]] Id unlock_for_level_clear(int level_index);

/** Bonus engine when every level has been cleared at least once. */
[[nodiscard]] Id all_tracks_bonus_engine();

[[nodiscard]] constexpr int portrait_frame(const Id id)
{
    return int(id);
}

[[nodiscard]] int coach_count(Id id);

[[nodiscard]] bool is_playable(Id id);

} // namespace engine

#endif
