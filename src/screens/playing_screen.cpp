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


void Game::_show_gameplay()
{
    _set_splash_visible(false);
    _banner.set_visible(false);
    _menu_row.set_visible(false);
    _menu_arrow.set_visible(false);
    _track_card.set_visible(false);
    _hatt.set_visible(false);
    _panel.set_visible(true);
    _panel.set_item(bn::sprite_items::panel, game_ui::PANEL_HUD);
    _panel.set_position(0, 66);
    _label.set_visible(true);
    _label.set_position(0, 68);
    _label.set_item(bn::sprite_items::label, game_ui::LABEL_BUILD);
    _hint_secondary.set_visible(false);
    _update_build_hints();
    _cursor.set_visible(true);
    _train.set_visible(false);
    _backdrop.set_backdrop_visible(true);
    _backdrop.set_scenery_visible(true);
}

void Game::_update_playing()
{
    if(_play_mode == PlayMode::build)
    {
        _update_build();
        _update_build_hints();
    }
    else
    {
        _update_build_hints();
        _update_test();
    }

    _sync_graph_sprites();
    _update_preview();
    _update_hud();
    _update_toast();
    _backdrop.animate_water(_frame);
}

void Game::_set_counter(bn::vector<bn::sprite_ptr, 2>& digits, int value, bn::fixed x_base)
{
    const int clamped = value < 0 ? 0 : (value > 99 ? 99 : value);
    digits[0].set_item(bn::sprite_items::digit, clamped / 10);
    digits[0].set_position(x_base, 66);
    digits[0].set_visible(_screen == Screen::playing);
    digits[1].set_item(bn::sprite_items::digit, clamped % 10);
    digits[1].set_position(x_base + 8, 66);
    digits[1].set_visible(_screen == Screen::playing);
}

void Game::_show_toast(game_ui::ToastKind kind)
{
    _toast_kind = kind;
    _toast_timer = game_ui::TOAST_FRAMES;
    _toast.set_visible(_screen == Screen::playing);

    switch(kind)
    {
        case game_ui::ToastKind::dup:
            _toast.set_item(bn::sprite_items::toast, game_ui::TOAST_DUP);
            break;

        case game_ui::ToastKind::weak:
            _toast.set_item(bn::sprite_items::toast, game_ui::TOAST_WEAK);
            break;

        case game_ui::ToastKind::deleted:
            _toast.set_item(bn::sprite_items::toast, game_ui::TOAST_DEL);
            break;

        default:
            break;
    }
}

void Game::_update_toast()
{
    if(_toast_timer <= 0 || _screen != Screen::playing)
    {
        _toast.set_visible(false);
        _toast_kind = game_ui::ToastKind::none;
        return;
    }

    --_toast_timer;
    _toast.set_visible(true);
}

void Game::_update_preview()
{
    if(_screen != Screen::playing || _play_mode != PlayMode::build || !_selected)
    {
        _preview_strut.set_visible(false);
        return;
    }

    const auto& nodes = _bridge.nodes();
    const bn::fixed_point from = nodes[_selected.value()].position;
    bn::fixed_point to = _cursor_pos;
    bn::fixed max_tension = 0;
    bool show = false;

    const bn::optional<int> hovered = _bridge.find_node_near(_cursor_pos);

    if(hovered && hovered.value() != _selected.value())
    {
        const bridge::StrutPreview preview =
            _bridge.preview_strut(_selected.value(), hovered.value(), _connect_role);

        if(preview.status == bridge::StrutPreviewStatus::ok)
        {
            to = nodes[hovered.value()].position;
            max_tension = preview.max_tension;
            show = true;
        }
    }
    else if(!hovered)
    {
        to = game::snap_point(_cursor_pos);
        const bn::fixed length = physics::distance(from, to);

        if(length > 0)
        {
            max_tension = physics::max_tension_for_length(length);
            show = true;
        }
    }

    if(show)
    {
        const bn::fixed load = _max_train_load();
        const bool weak = max_tension < load;
        int frame = view::STRUT_PREVIEW_OK;

        if(_connect_role == bridge::LinkRole::brace)
        {
            frame = weak ? view::STRUT_BRACE_HOT : view::STRUT_BRACE_IDLE;
        }
        else
        {
            frame = weak ? view::STRUT_PREVIEW_WEAK : view::STRUT_PREVIEW_OK;
        }

        view::position_strut_sprite(_preview_strut, from, to, frame);
        return;
    }

    _preview_strut.set_visible(false);
}

void Game::_update_build_hints()
{
    if(_screen != Screen::playing || _play_mode != PlayMode::build)
    {
        _hint_label.set_visible(false);
        return;
    }

    if(!_build_hints_seen)
    {
        ++_build_hint_timer;

        if(_build_hint_timer >= game_ui::BUILD_HINT_FRAMES ||
           bn::keypad::any_pressed())
        {
            _build_hints_seen = true;
        }

        const int phase = (_build_hint_timer / 60) % 3;
        int frame = game_ui::LABEL_HINT_NODE;

        if(phase == 1)
        {
            frame = game_ui::LABEL_HINT_TEST;
        }
        else if(phase == 2)
        {
            frame = game_ui::LABEL_HINT_PAUSE;
        }

        _hint_label.set_visible(true);
        _hint_label.set_position(0, 52);
        _hint_label.set_item(bn::sprite_items::label, frame);
        return;
    }

    _hint_label.set_visible(true);
    _hint_label.set_position(0, 52);
    _hint_label.set_item(bn::sprite_items::label, game_ui::LABEL_HINT_PAUSE);
}

void Game::_update_build()
{
    bn::fixed speed = bn::keypad::r_held() ? game_ui::CURSOR_FAST : game_ui::CURSOR_SPEED;
    int axes = 0;

    if(bn::keypad::left_held() || bn::keypad::right_held())
    {
        ++axes;
    }

    if(bn::keypad::up_held() || bn::keypad::down_held())
    {
        ++axes;
    }

    if(axes > 1)
    {
        speed = speed * 7 / 10;
    }

    if(bn::keypad::left_held())
    {
        _cursor_pos.set_x(_cursor_pos.x() - speed);
    }

    if(bn::keypad::right_held())
    {
        _cursor_pos.set_x(_cursor_pos.x() + speed);
    }

    if(bn::keypad::up_held())
    {
        _cursor_pos.set_y(_cursor_pos.y() - speed);
    }

    if(bn::keypad::down_held())
    {
        _cursor_pos.set_y(_cursor_pos.y() + speed);
    }

    _cursor_pos.set_x(game_ui::clamp(_cursor_pos.x(), -112, 112));
    _cursor_pos.set_y(game_ui::clamp(_cursor_pos.y(), -72, game::UNDERGROUND_Y));

    const bn::optional<int> hovered = _bridge.find_node_near(_cursor_pos);
    bn::fixed_point draw_at = hovered ?
        _bridge.nodes()[hovered.value()].position : game::snap_point(_cursor_pos);

    _cursor.set_position(draw_at);
    _cursor.set_visible(true);

    if(!hovered)
    {
        _ghost.set_position(draw_at);
        _ghost.set_visible(true);
    }
    else
    {
        _ghost.set_visible(false);
    }

    if(bn::keypad::a_pressed())
    {
        _handle_confirm(hovered, draw_at);
    }

    if(bn::keypad::b_pressed())
    {
        if(hovered && _bridge.is_player_node(hovered.value()))
        {
            if(_bridge.remove_player_node(hovered.value()) == bridge::RemoveNodeResult::ok)
            {
                if(_selected && _selected.value() == hovered.value())
                {
                    _selected.reset();
                }
                else if(_selected && _selected.value() > hovered.value())
                {
                    _selected = _selected.value() - 1;
                }

                _show_toast(game_ui::ToastKind::deleted);
            }
        }
        else
        {
            _selected.reset();
        }
    }

    if(bn::keypad::r_pressed() && !bn::keypad::left_held() && !bn::keypad::right_held() &&
       !bn::keypad::up_held() && !bn::keypad::down_held())
    {
        _cycle_connect_role();
    }

    if(bn::keypad::l_pressed())
    {
        const bridge::UndoResult undone = _bridge.undo_last_edit();

        if(undone == bridge::UndoResult::removed_node)
        {
            _selected.reset();
        }
    }

    if(bn::keypad::select_pressed())
    {
        _enter_pause();
    }

    if(bn::keypad::start_pressed())
    {
        _enter_test();
    }
}

void Game::_handle_confirm(bn::optional<int> hovered, bn::fixed_point place_at)
{
    if(_selected && hovered && _selected.value() == hovered.value())
    {
        _selected.reset();
        return;
    }

    int node_id;

    if(hovered)
    {
        node_id = hovered.value();
    }
    else
    {
        if(_bridge.player_node_count() >= _bridge.player_node_budget())
        {
            _show_toast(game_ui::ToastKind::full);
            return;
        }

        const bn::fixed_point snapped = game::snap_point(place_at);
        const bn::optional<int> created =
            _bridge.add_node(snapped.x(), snapped.y(), false);

        if(!created)
        {
            _show_toast(game_ui::ToastKind::full);
            return;
        }

        node_id = created.value();
    }

    if(_selected && _selected.value() != node_id)
    {
        const bridge::StrutPreview preview =
            _bridge.preview_strut(_selected.value(), node_id, _connect_role);

        switch(preview.status)
        {
            case bridge::StrutPreviewStatus::duplicate:
                _show_toast(game_ui::ToastKind::dup);
                break;

            case bridge::StrutPreviewStatus::struts_full:
                _show_toast(game_ui::ToastKind::full);
                break;

            case bridge::StrutPreviewStatus::ok:
                if(preview.max_tension < _max_train_load())
                {
                    _show_toast(game_ui::ToastKind::weak);
                }

                (void)_bridge.add_strut(_selected.value(), node_id, _connect_role);
                break;

            default:
                break;
        }
    }

    _selected = node_id;
}

void Game::_cycle_connect_role()
{
    if(!_bridge.allows_link_role(bridge::LinkRole::brace))
    {
        _connect_role = bridge::LinkRole::track;
        return;
    }

    _connect_role = _connect_role == bridge::LinkRole::track ?
        bridge::LinkRole::brace : bridge::LinkRole::track;
    bn::sound_items::ui.play();
}

int Game::_build_tool_label() const
{
    if(_bridge.allows_link_role(bridge::LinkRole::brace))
    {
        return _connect_role == bridge::LinkRole::track ? game_ui::LABEL_TRACK : game_ui::LABEL_ROD;
    }

    return game_ui::LABEL_BUILD;
}

void Game::_enter_test()
{
    _fail_reveal_timer = 0;
    _play_mode = PlayMode::test;
    _selected.reset();
    _cursor.set_visible(false);
    _ghost.set_visible(false);
    _ring.set_visible(false);
    _preview_strut.set_visible(false);
    _train.set_visible(true);
    _train_prev_x = _bridge.train_position().x();
    (void)_bridge.begin_test();
    game_ui::set_train_sprite_position(_train, _bridge.train_position());
    _chuff_cooldown = 0;

    if(_bridge.status() == bridge::SimStatus::no_path)
    {
        _audio.play(audio::MusicCue::none);
        bn::sound_items::delay.play();
        _no_go_timer = 60;
        _leave_test();
        return;
    }
    else
    {
        _audio.play(audio::MusicCue::full_steam);
        bn::sound_items::peep.play();
    }
}

void Game::_leave_test()
{
    _fail_reveal_timer = 0;
    _bridge.end_test();
    _play_mode = PlayMode::build;
    _train.set_visible(false);
    _cursor.set_visible(true);
    _cursor.set_position(_cursor_pos);
    _audio.play(audio::MusicCue::workshop);
}

void Game::_update_test()
{
    const bn::fixed prev_x = _train.position().x();
    const bridge::SimStatus prev_status = _bridge.status();
    (void)_bridge.update();
    game_ui::set_train_sprite_position(_train, _bridge.train_position());

    const bridge::SimStatus status = _bridge.status();

    if(status == bridge::SimStatus::failed_fall && prev_status == bridge::SimStatus::running)
    {
        _fail_reveal_timer = 12;
        _trigger_shake();
        _spawn_dust(_train.position());
    }

    if(_train.position().x() > prev_x + bn::fixed(1) / 16)
    {
        _train_facing_right = true;
    }
    else if(_train.position().x() < prev_x - bn::fixed(1) / 16)
    {
        _train_facing_right = false;
    }

    _train.set_horizontal_flip(!_train_facing_right);
    _train.set_visible(_train.position().y() < bridge::FALL_Y_LIMIT);

    if(status == bridge::SimStatus::success)
    {
        _show_level_complete();
        return;
    }

    if(status == bridge::SimStatus::failed_fall)
    {
        if(_fail_reveal_timer > 0)
        {
            --_fail_reveal_timer;

            if(const bridge::SnapFlash* flash = _bridge.snap_flash())
            {
                view::position_strut_sprite(_preview_strut, flash->a, flash->b, view::STRUT_SNAP);
                _preview_strut.set_palette(
                    bn::sprite_items::strut.palette_item().create_palette());
                _preview_strut.set_visible(true);
            }
            else
            {
                _preview_strut.set_visible(false);
            }

            return;
        }

        _show_level_failed();
        return;
    }

    if(bn::keypad::start_pressed() || bn::keypad::b_pressed())
    {
        _leave_test();
    }
}

void Game::_update_hud()
{
    int frame = game_ui::LABEL_BUILD;

    if(_no_go_timer > 0)
    {
        --_no_go_timer;
        frame = game_ui::LABEL_NO_GO;
    }
    else if(_play_mode == PlayMode::test)
    {
        switch(_bridge.status())
        {
            case bridge::SimStatus::success:
                frame = game_ui::LABEL_USEFUL;
                break;

            case bridge::SimStatus::failed_fall:
                frame = game_ui::LABEL_ASHES;
                break;

            case bridge::SimStatus::no_path:
                frame = game_ui::LABEL_NO_GO;
                break;

            default:
                frame = game_ui::LABEL_TEST;
                break;
        }
    }
    else if(_play_mode == PlayMode::build)
    {
        frame = _build_tool_label();
    }

    _label.set_item(bn::sprite_items::label, frame);
    _set_counter(_node_count_digits, _bridge.player_node_count(), -20);
    _set_counter(_strut_count_digits, _bridge.struts().size(), 12);
    _set_weight_digits(_max_train_load().integer(), true);
}
