#ifndef MORRIS_BRIDGE_GAME_UI_H
#define MORRIS_BRIDGE_GAME_UI_H

/**
 * @file
 * Shared UI constants and helpers for GBA screens.
 */

#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_sprite_builder.h"
#include "bn_sprite_ptr.h"

#include "game_constants.h"

namespace game_ui
{

constexpr bn::fixed CURSOR_SPEED = 3;
constexpr bn::fixed CURSOR_FAST = 6;
constexpr int CURSOR_REPEAT_DELAY = 10;
constexpr int CURSOR_REPEAT_RATE = 3;
constexpr int TOAST_FRAMES = 90;
constexpr int SHAKE_FRAMES = 10;
constexpr int DUST_LIFE = 24;
constexpr int MAX_DUST = 6;
constexpr int CHUFF_FRAMES = 10;

constexpr int TOAST_DUP = 0;
constexpr int TOAST_WEAK = 1;
constexpr int TOAST_DEL = 2;

constexpr int Z_CURSOR = -32767;
constexpr int Z_TOAST = -17;
constexpr int Z_DIGIT = -19;
constexpr int Z_RING = 4;
constexpr int Z_GHOST = 6;
constexpr int Z_STRUT_PREVIEW = 9;

enum class ToastKind
{
    none,
    dup,
    weak,
    deleted,
    full,
    undo,
};

struct DustPuff
{
    bn::sprite_ptr sprite;
    bn::fixed vy = 0;
    int life = 0;
};

constexpr int LABEL_BUILD = 0;
constexpr int LABEL_TEST = 1;
constexpr int LABEL_DELAY = 2;
constexpr int LABEL_USEFUL = 3;
constexpr int LABEL_NO_GO = 4;
constexpr int LABEL_PEEP = 5;
constexpr int LABEL_ASHES = 6;
constexpr int LABEL_START = 7;
constexpr int LABEL_HINT_NODE = 8;
constexpr int LABEL_HINT_WELD = 9;
constexpr int LABEL_HINT_PAUSE = 10;
constexpr int LABEL_HINT_TEST = 9;

constexpr int BUILD_HINT_FRAMES = 240;

/// Squared pixels of cursor travel before hold-A counts as a weld drag.
constexpr int WELD_DRAG_SQ = 36;
/// Minimum squared length before hold-A plants the next joint (24 px).
constexpr int CHAIN_WELD_MIN_SQ = 576;

constexpr int LABEL_CHUFF = 11;
constexpr int LABEL_TOOT = 12;
constexpr int LABEL_OOPS = 13;
constexpr int LABEL_WAIT = 14;
constexpr int LABEL_AGAIN = 15;
constexpr int LABEL_NEW = 16;
constexpr int LABEL_TRACK = 17;
constexpr int LABEL_ROD = 18;
constexpr int LABEL_INTRO_BASE = 19;

constexpr int LABEL_LISTEN = 35;
constexpr int LABEL_CONFUS = 36;
constexpr int LABEL_LOOP = 37;
constexpr int LABEL_THIN = 38;

constexpr int BANNER_LEVEL_SELECT = 0;
constexpr int BANNER_HATT_OK = 1;
constexpr int BANNER_DELAY = 2;
constexpr int BANNER_HATT_PEEP = 3;
constexpr int BANNER_DELAY_LISTEN = 4;
constexpr int BANNER_ENGINE_SELECT = 5;

constexpr int PANEL_HUD = 0;
constexpr int PANEL_DIALOG = 1;
constexpr int PANEL_SELECT = 2;

constexpr int HATT_STERN = 0;
constexpr int HATT_FURIOUS = 1;

constexpr int MENU_PAUSE_BASE = 3;
constexpr int MENU_ENGINE_BASE = 7;

inline bn::sprite_ptr make_sprite(const bn::sprite_item& item, bn::fixed x, bn::fixed y, int z_order)
{
    bn::sprite_builder builder(item);
    builder.set_position(x, y);
    builder.set_z_order(z_order);
    return builder.release_build();
}

inline bn::fixed clamp(bn::fixed value, bn::fixed lo, bn::fixed hi)
{
    if(value < lo)
    {
        return lo;
    }

    if(value > hi)
    {
        return hi;
    }

    return value;
}

inline void set_train_sprite_position(bn::sprite_ptr& train, bn::fixed_point track_position)
{
    train.set_position(game::train_display_position(track_position));
}

} // namespace game_ui

#endif
