#ifndef MORRIS_BRIDGE_GAME_CONSTANTS_H
#define MORRIS_BRIDGE_GAME_CONSTANTS_H

/**
 * @file
 * Shared layout constants for Thomas's Bridge.
 *
 * Butano sprites use center-origin coordinates. Ground tiles are placed with
 * their center at GROUND_TILE_Y; the visible rail surface sits TRACK_Y.
 */

#include "bn_fixed.h"
#include "bn_fixed_point.h"

namespace game
{

/// Center Y of ground scenery tiles (32×16 sprites).
inline constexpr bn::fixed GROUND_TILE_Y = 56;

/// Default flat-track rail height — wheels and joints align here.
inline constexpr bn::fixed TRACK_Y = 52;

/// Deepest build-grid row for pier foundations below the deck.
inline constexpr bn::fixed UNDERGROUND_Y = 64;

/// 8 px build grid (vision: 8×8 GBA snap).
inline constexpr bn::fixed GRID_STEP = 8;

/// Hold-A chain-draw plants the next joint at least this far from the last.
inline constexpr bn::fixed CHAIN_WELD_MIN = 24;

/// Train sprite center sits above node joints so wheels rest on the rail.
inline constexpr bn::fixed TRAIN_Y_OFFSET = -6;

/**
 * @brief Snap one axis to the build grid.
 */
[[nodiscard]] inline bn::fixed snap_axis(bn::fixed value)
{
    const int raw = value.shift_integer();
    const int step = GRID_STEP.shift_integer();
    const int snapped = ((raw + step / 2) / step) * step;
    return bn::fixed(snapped);
}

/**
 * @brief Snap a cursor position to the build grid.
 */
[[nodiscard]] inline bn::fixed_point snap_point(bn::fixed_point point)
{
    return bn::fixed_point(snap_axis(point.x()), snap_axis(point.y()));
}

/**
 * @brief Display position for the train sprite (wheels on TRACK_Y).
 */
[[nodiscard]] inline bn::fixed_point train_display_position(bn::fixed_point track_position)
{
    return bn::fixed_point(track_position.x(), track_position.y() + TRAIN_Y_OFFSET);
}

} // namespace game

#endif
