#include "game.h"

#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_music.h"
#include "bn_sram.h"
#include "bn_sound_items.h"

#include "bn_sprite_items_banner.h"
#include "bn_sprite_items_cursor.h"
#include "bn_sprite_items_digit.h"
#include "bn_sprite_items_ghost.h"
#include "bn_sprite_items_hatt.h"
#include "bn_sprite_items_label.h"
#include "bn_sprite_items_logo.h"
#include "bn_sprite_items_menu.h"
#include "bn_sprite_items_panel.h"
#include "bn_sprite_items_portrait.h"
#include "bn_sprite_items_ring.h"
#include "bn_sprite_items_splash.h"
#include "bn_sprite_items_strut.h"
#include "bn_sprite_items_toast.h"
#include "bn_sprite_items_train.h"

#include "audio_director.h"
#include "game_constants.h"
#include "graph_view.h"
#include "level.h"
#include "physics.h"
#include "save_data.h"


void Game::_update_pause()
{
    if(_update_menu())
    {
        switch(_menu_choice())
        {
            case game_ui::OPTION_RESUME:
                _leave_pause();
                break;

            case game_ui::OPTION_RESTART:
                _bridge.reset_level();
                _selected.reset();
                _cursor_pos = _bridge.nodes()[bridge::START_NODE].position;
                _cursor_pos.set_x(_cursor_pos.x() + 16);
                _leave_pause();
                _sync_graph_sprites();
                _audio.play(audio::MusicCue::workshop);
                break;

            case game_ui::OPTION_LEVELS:
                _close_menu();
                _show_level_select();
                break;

            case game_ui::OPTION_TITLE:
                _close_menu();
                _show_title();
                break;

            default:
                break;
        }

        return;
    }

    if(bn::keypad::b_pressed() || bn::keypad::select_pressed())
    {
        _leave_pause();
    }
}
