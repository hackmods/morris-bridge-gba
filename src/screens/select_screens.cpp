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


void Game::_show_level_select()
{
    _screen = Screen::level_select;
    _hide_gameplay();
    _backdrop.reset_scenery_layout();
    _backdrop.apply_parallax(_parallax_shift, _bridge.level_def());
    _set_splash_visible(false);
    _backdrop.set_backdrop_visible(true);
    _banner.set_visible(true);
    _banner.set_item(bn::sprite_items::banner, game_ui::BANNER_LEVEL_SELECT);
    _banner.set_position(0, -52);
    _panel.set_visible(true);
    _panel.set_item(bn::sprite_items::panel, game_ui::PANEL_SELECT);
    _panel.set_position(0, 16);
    _menu_row.set_visible(true);
    _menu_row.set_item(bn::sprite_items::menu, _selected_level);
    _menu_row.set_position(0, 4);
    _menu_arrow.set_visible(true);
    _menu_arrow.set_position(-38, 12);
    _label.set_visible(false);
    _hint_label.set_visible(false);
    _hint_secondary.set_visible(false);
    _train.set_visible(true);
    _train.set_position(-56, 44);
    _sync_train_sprite();
    _hatt.set_visible(true);
    _hatt.set_position(56, 44);
    _hatt.set_item(bn::sprite_items::hatt, game_ui::HATT_STERN);
    _portrait.set_visible(false);
    _sync_level_select_stars();
    _sync_level_select_flavor();
    _node_count_digits[0].set_visible(false);
    _node_count_digits[1].set_visible(false);
    _strut_count_digits[0].set_visible(false);
    _strut_count_digits[1].set_visible(false);
}

void Game::_show_engine_select()
{
    _screen = Screen::engine_select;
    _hide_gameplay();
    _set_splash_visible(false);
    _backdrop.set_backdrop_visible(true);
    _banner.set_visible(true);
    _banner.set_item(bn::sprite_items::banner, game_ui::BANNER_ENGINE_SELECT);
    _banner.set_position(0, -48);
    _panel.set_visible(true);
    _panel.set_item(bn::sprite_items::panel, game_ui::PANEL_SELECT);
    _panel.set_position(0, 20);
    _menu_row.set_visible(true);
    _menu_row.set_item(bn::sprite_items::menu, engine::menu_frame(_selected_engine));
    _menu_row.set_position(0, 8);
    _menu_arrow.set_visible(true);
    _menu_arrow.set_position(-38, 16);
    _label.set_visible(false);
    _train.set_visible(true);
    _train.set_position(-56, game::TRACK_Y + game::TRAIN_Y_OFFSET);
    _sync_train_sprite();
    _set_weight_digits(engine::profile(_selected_engine).weight.integer(), true);
    _portrait.set_visible(true);
    _portrait.set_position(56, 44);
    _portrait.set_item(bn::sprite_items::portrait, engine::portrait_frame(_selected_engine));
    _hatt.set_visible(true);
    _hatt.set_position(56, 44);
    _hatt.set_item(bn::sprite_items::hatt, game_ui::HATT_STERN);
    _node_count_digits[0].set_visible(false);
    _node_count_digits[1].set_visible(false);
    _strut_count_digits[0].set_visible(false);
    _strut_count_digits[1].set_visible(false);
    _audio.play(audio::MusicCue::yard);
}

void Game::_show_level_intro(const int level_index)
{
    const level::Def& def = level::get(level_index);
    _screen = Screen::level_intro;
    _hide_gameplay();
    _set_splash_visible(false);
    _backdrop.set_backdrop_visible(true);
    _banner.set_visible(true);
    _banner.set_item(bn::sprite_items::banner, game_ui::BANNER_LEVEL_SELECT);
    _banner.set_position(0, -40);
    _panel.set_visible(true);
    _panel.set_item(bn::sprite_items::panel, game_ui::PANEL_DIALOG);
    _panel.set_position(0, 48);
    _menu_row.set_visible(false);
    _menu_arrow.set_visible(false);
    _cursor.set_visible(false);
    _train.set_visible(true);
    _train.set_position(-56, game::TRACK_Y + game::TRAIN_Y_OFFSET);
    _sync_train_sprite();
    _hatt.set_visible(true);
    _hatt.set_position(56, 44);
    _hatt.set_item(bn::sprite_items::hatt, game_ui::HATT_STERN);
    _portrait.set_visible(false);
    _set_weight_digits(0, false);
    _node_count_digits[0].set_visible(false);
    _node_count_digits[1].set_visible(false);
    _strut_count_digits[0].set_visible(false);
    _strut_count_digits[1].set_visible(false);

    const int label_index = game_ui::LABEL_INTRO_BASE + int(def.intro_dialog_id - 1) * 2;
    _label.set_visible(true);
    _label.set_position(0, 44);
    _label.set_item(bn::sprite_items::label, label_index);
    _hint_label.set_visible(true);
    _hint_label.set_position(0, 54);
    _hint_label.set_item(bn::sprite_items::label, label_index + 1);
    _hint_secondary.set_visible(false);
    _audio.play(audio::MusicCue::yard);
}

void Game::_update_level_select()
{
    if(_dir_repeat(bn::keypad::up_pressed(), bn::keypad::up_held(), _menu_up_counter))
    {
        _step_level(-1);
        bn::sound_items::ui.play();
    }

    if(_dir_repeat(bn::keypad::down_pressed(), bn::keypad::down_held(), _menu_down_counter))
    {
        _step_level(1);
        bn::sound_items::ui.play();
    }

    if(bn::keypad::a_pressed() || bn::keypad::start_pressed())
    {
        if(! _progress.is_level_unlocked(_selected_level))
        {
            return;
        }

        _pending_level = _selected_level;

        if(! _progress.is_engine_unlocked(_selected_engine))
        {
            _selected_engine = engine::default_engine();
        }

        _show_engine_select();
        return;
    }

    if(bn::keypad::b_pressed())
    {
        _show_title();
    }
}

void Game::_update_engine_select()
{
    if(_dir_repeat(bn::keypad::up_pressed(), bn::keypad::up_held(), _menu_up_counter))
    {
        _step_engine(-1);
    }

    if(_dir_repeat(bn::keypad::down_pressed(), bn::keypad::down_held(), _menu_down_counter))
    {
        _step_engine(1);
    }

    if(bn::keypad::a_pressed() || bn::keypad::start_pressed())
    {
        if(_progress.is_engine_unlocked(_selected_engine))
        {
            _begin_pending_level();
        }

        return;
    }

    if(bn::keypad::b_pressed())
    {
        _show_level_select();
    }
}

void Game::_update_level_intro()
{
    if(bn::keypad::a_pressed() || bn::keypad::start_pressed() || bn::keypad::b_pressed())
    {
        _progress.mark_intro_seen(_pending_level);
        _persist_progress();
        _start_level(_pending_level);
    }
}

void Game::_sync_level_select_flavor()
{
    if(_lore_rare(6 + _selected_level))
    {
        int label = game_ui::LABEL_PEEP;

        switch(_selected_level)
        {
            case 1:
                label = game_ui::LABEL_WAIT;
                break;

            case 2:
                label = game_ui::LABEL_TOOT;
                break;

            default:
                break;
        }

        _hint_label.set_visible(true);
        _hint_label.set_position(0, 52);
        _hint_label.set_item(bn::sprite_items::label, label);
        return;
    }

    const level::Def& def = level::get(_selected_level);

    if(def.intro_dialog_id > 0)
    {
        const int label = game_ui::LABEL_INTRO_BASE + int(def.intro_dialog_id - 1) * 2;
        _hint_label.set_visible(true);
        _hint_label.set_position(0, 52);
        _hint_label.set_item(bn::sprite_items::label, label);
        return;
    }

    _hint_label.set_visible(false);
}

void Game::_sync_level_select_stars()
{
    const uint8_t stars = _progress.best_stars(_selected_level);
    _set_weight_digits(stars, stars > 0);
}
