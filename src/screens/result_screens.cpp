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


void Game::_show_level_complete()
{
    _screen = Screen::level_complete;
    _set_splash_visible(false);
    _newly_unlocked = engine::none;
    _show_unlock_flash = false;

    engine::Id unlocked = engine::none;

    if(_progress.on_level_cleared(_bridge.current_level(), unlocked))
    {
        _newly_unlocked = unlocked;
        _show_unlock_flash = true;
    }

    _last_run_score = session::compute_run_score(_bridge, _bridge.level_def());
    _progress.record_level_result(_bridge.current_level(), _last_run_score.stars,
                                  uint16_t(_last_run_score.total_cost));
    _persist_progress();

    _banner.set_visible(true);
    _banner.set_item(bn::sprite_items::banner, _pick_win_banner());
    _banner.set_position(0, -28);
    _panel.set_visible(true);
    _panel.set_item(bn::sprite_items::panel, game_ui::PANEL_DIALOG);
    _panel.set_position(0, 48);
    _label.set_visible(true);
    _label.set_position(0, 50);
    _label.set_item(bn::sprite_items::label,
                    _show_unlock_flash ? game_ui::LABEL_NEW : _pick_win_label());
    _hint_label.set_visible(false);
    _hint_secondary.set_visible(false);
    _menu_arrow.set_visible(false);
    _cursor.set_visible(false);
    _train.set_visible(true);
    game_ui::set_train_sprite_position(_train, _bridge.train_position());
    _audio.play(audio::MusicCue::none);
    bn::sound_items::useful.play();
    bn::sound_items::whistle.play();
    _hatt.set_visible(true);
    _hatt.set_position(-56, 8);
    _hatt.set_item(bn::sprite_items::hatt, game_ui::HATT_STERN);
    _sync_train_sprite();
    _set_weight_digits(_last_run_score.stars, _last_run_score.stars > 0);
}

void Game::_show_level_failed()
{
    _screen = Screen::level_failed;
    _set_splash_visible(false);
    _banner.set_visible(true);
    _banner.set_item(bn::sprite_items::banner, _pick_fail_banner());
    _banner.set_position(0, -28);
    _panel.set_visible(true);
    _panel.set_item(bn::sprite_items::panel, game_ui::PANEL_DIALOG);
    _panel.set_position(0, 48);
    _label.set_visible(true);
    _label.set_position(0, 50);
    _label.set_item(bn::sprite_items::label, _pick_fail_label());
    _hint_label.set_visible(false);
    _hint_secondary.set_visible(false);
    _cursor.set_visible(false);
    _train.set_visible(_train.position().y() < bridge::FALL_Y_LIMIT);
    _sync_train_sprite();
    _audio.play(audio::MusicCue::none);
    bn::sound_items::snap.play();
    bn::sound_items::delay.play();
    bn::sound_items::splash.play();
    _hatt.set_visible(true);
    _hatt.set_position(-56, 8);
    _hatt.set_item(bn::sprite_items::hatt, game_ui::HATT_FURIOUS);
    _set_weight_digits(0, false);
}

void Game::_show_all_complete()
{
    _screen = Screen::all_complete;
    _set_splash_visible(false);
    _banner.set_visible(true);
    const bool epilogue_peep = _lore_rare(5);

    if(epilogue_peep)
    {
        _banner.set_item(bn::sprite_items::banner, game_ui::BANNER_HATT_PEEP);
        _label.set_item(bn::sprite_items::label, game_ui::LABEL_PEEP);
    }
    else
    {
        _banner.set_item(bn::sprite_items::banner, game_ui::BANNER_HATT_OK);
        _label.set_item(bn::sprite_items::label, game_ui::LABEL_USEFUL);
    }

    _banner.set_position(0, -36);
    _panel.set_visible(true);
    _panel.set_item(bn::sprite_items::panel, game_ui::PANEL_DIALOG);
    _panel.set_position(0, 40);
    _label.set_visible(true);
    _label.set_position(0, 42);
    _hint_label.set_visible(false);
    _hint_secondary.set_visible(false);
    _menu_row.set_visible(false);
    _menu_arrow.set_visible(false);
    _train.set_visible(true);
    _train.set_position(0, game::TRACK_Y + game::TRAIN_Y_OFFSET);
    _hatt.set_visible(true);
    _hatt.set_position(56, 16);
    _hatt.set_item(bn::sprite_items::hatt, game_ui::HATT_STERN);
    _audio.play(audio::MusicCue::island_line);
    bn::sound_items::useful.play();

    if(epilogue_peep)
    {
        bn::sound_items::whistle.play();
    }
}

void Game::_update_level_complete()
{
    if(bn::keypad::a_pressed() || bn::keypad::start_pressed())
    {
        _bridge.end_test();
        _play_mode = PlayMode::build;

        if(_bridge.current_level() + 1 >= bridge::LEVEL_COUNT)
        {
            _show_all_complete();
        }
        else
        {
            _pending_level = _bridge.current_level() + 1;
            _start_level(_pending_level);
        }

        return;
    }

    if(bn::keypad::b_pressed())
    {
        _bridge.end_test();
        _play_mode = PlayMode::build;
        _show_level_select();
    }
}

void Game::_update_level_failed()
{
    if(bn::keypad::a_pressed() || bn::keypad::start_pressed() || bn::keypad::b_pressed())
    {
        _leave_test();
    }
}

void Game::_update_all_complete()
{
    if(bn::keypad::a_pressed() || bn::keypad::start_pressed() || bn::keypad::b_pressed())
    {
        _show_title();
    }
}
