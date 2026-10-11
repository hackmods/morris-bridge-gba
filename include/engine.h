#ifndef MORRIS_BRIDGE_ENGINE_H
#define MORRIS_BRIDGE_ENGINE_H

/**
 * @file
 * Playable engines and the trains they pull.
 *
 * `Id` values are save-file bits and sprite-sheet rows, so new engines are
 * appended; `ROSTER` is the order the player sees (by number, 1 to 6).
 */

#include <stdint.h>

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
    henry,
    count
};

/// Invalid / no-engine sentinel (not a playable Id).
inline constexpr Id none = Id::count;

/// Engine-select order: No. 1 Thomas through No. 6 Percy.
inline constexpr Id ROSTER[int(Id::count)] = {
    Id::thomas, Id::edward, Id::henry, Id::gordon, Id::james, Id::percy,
};

/// What rolls behind an engine. Values are frames of the `car` sprite sheet.
enum class CarKind : uint8_t
{
    tender_blue,
    tender_green,
    tender_red,
    annie,
    clarabel,
    coach,
    truck,   ///< Troublesome Truck: its load jolts as it bumps along.
    banker,  ///< Edward pushing from behind (drawn from the engine sheet).
};

struct Car
{
    CarKind kind;
    bn::fixed weight;
};

inline constexpr int FRAMES_PER_ENGINE = 2;
inline constexpr int MENU_ENGINE_BASE = 7;

/// Most cars an engine pulls by itself.
inline constexpr int MAX_OWN_CARS = 3;
/// Most cars in any train: an engine's own, plus a banker on the hill.
inline constexpr int MAX_CARS = MAX_OWN_CARS + 1;

/// Distance between one vehicle and the next (one sprite length).
inline constexpr bn::fixed CAR_SPACING = 26;

/// A banking engine's weight on the rear of the train.
inline constexpr bn::fixed BANKER_WEIGHT = 4;

struct Profile
{
    bn::fixed weight;
    bn::fixed speed;
    int car_count;
    Car cars[MAX_OWN_CARS];
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

/** Engine plus everything it pulls: the whole load a bridge has to carry. */
[[nodiscard]] bn::fixed total_weight(Id id);

/** Position of @p id in `ROSTER`. */
[[nodiscard]] int roster_index(Id id);

[[nodiscard]] bool is_playable(Id id);

} // namespace engine

#endif
