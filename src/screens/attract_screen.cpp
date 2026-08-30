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


void Game::_show_title()
{
    _screen = Screen::title;
    _play_mode = PlayMode::build;
    _hide_gameplay();
    _backdrop.reset_scenery_layout();
    _backdrop.apply_parallax(_parallax_shift, _bridge.level_def());
    _backdrop.set_backdrop_visible(false);
    _set_splash_visible(true);
    _logo.set_visible(true);
    _logo.set_position(0, -52);
    _banner.set_visible(false);
    _panel.set_visible(true);
    _panel.set_item(bn::sprite_items::panel, game_ui::PANEL_DIALOG);
    _panel.set_position(0, 58);
    _label.set_visible(true);
    _label.set_position(0, 60);
    _label.set_item(bn::sprite_items::label, game_ui::LABEL_START);
    _hint_label.set_visible(true);
    _hint_label.set_position(0, 42);
    _hint_label.set_item(bn::sprite_items::label, game_ui::LABEL_HINT_NODE);
    _hint_secondary.set_visible(true);
    _hint_secondary.set_position(0, 50);
    _hint_secondary.set_item(bn::sprite_items::label, game_ui::LABEL_HINT_WELD);
    _menu_row.set_visible(false);
    _menu_arrow.set_visible(false);
    _track_card.set_visible(false);
    _train.set_visible(false);
    _hatt.set_visible(false);
    _audio.play(audio::MusicCue::island_line);
}

void Game::_update_title()
{
    _label.set_visible((_frame / 18) % 2 == 0);

    if(bn::keypad::start_pressed() || bn::keypad::a_pressed())
    {
        _show_level_select();
    }
}
