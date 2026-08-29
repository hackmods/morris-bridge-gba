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
    if(_dir_repeat(bn::keypad::up_pressed(), bn::keypad::up_held(), _menu_up_counter))
    {
        _pause_option = PauseOption((int(_pause_option) + int(PauseOption::count) - 1) %
                                    int(PauseOption::count));
        bn::sound_items::ui.play();
    }

    if(_dir_repeat(bn::keypad::down_pressed(), bn::keypad::down_held(), _menu_down_counter))
    {
        _pause_option = PauseOption((int(_pause_option) + 1) % int(PauseOption::count));
        bn::sound_items::ui.play();
    }

    _menu_row.set_item(bn::sprite_items::menu, game_ui::MENU_PAUSE_BASE + int(_pause_option));

    if(bn::keypad::a_pressed() || bn::keypad::start_pressed())
    {
        switch(_pause_option)
        {
            case PauseOption::resume:
                _leave_pause();
                break;

            case PauseOption::restart:
                _bridge.reset_level();
                _selected.reset();
                _cursor_pos = _bridge.nodes()[bridge::START_NODE].position;
                _cursor_pos.set_x(_cursor_pos.x() + 16);
                _leave_pause();
                _sync_graph_sprites();
                _audio.play(audio::MusicCue::workshop);
                break;

            case PauseOption::levels:
                _show_level_select();
                break;

            case PauseOption::title:
                _show_title();
                break;

            default:
                break;
        }
    }

    if(bn::keypad::b_pressed())
    {
        _leave_pause();
    }
}
