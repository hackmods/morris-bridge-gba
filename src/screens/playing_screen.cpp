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
#include "material.h"
#include "physics.h"
#include "run_score.h"
#include "save_data.h"


void Game::_show_gameplay()
{
    _set_splash_visible(false);
    _banner.set_visible(false);
    _menu_row.set_visible(false);
    _menu_arrow.set_visible(false);
    _track_card.set_visible(false);
    _hatt.set_visible(false);
    _top_bar.set_visible(true);
    _top_bar.set_position(0, game_ui::HUD_TOP_Y);
    _top_bar.set_item(bn::sprite_items::panel, game_ui::PANEL_HUD);
    _top_bar.set_vertical_scale(bn::fixed(1) / 4);
    _panel.set_visible(true);
    _panel.set_item(bn::sprite_items::panel, game_ui::PANEL_HUD);
    _panel.set_position(0, game_ui::HUD_BOTTOM_Y - 2);
    _panel.set_vertical_scale(bn::fixed(1) / 2);
    _label.set_visible(true);
    _label.set_position(-88, game_ui::HUD_TOP_Y);
    _label.set_item(bn::sprite_items::label, game_ui::LABEL_EDIT);
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
    const bn::fixed y = (_screen == Screen::playing) ? game_ui::HUD_TOP_Y : bn::fixed(66);
    digits[0].set_item(bn::sprite_items::digit, clamped / 10);
    digits[0].set_position(x_base, y);
    digits[0].set_visible(_screen == Screen::playing);
    digits[1].set_item(bn::sprite_items::digit, clamped % 10);
    digits[1].set_position(x_base + 8, y);
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
            _bridge.preview_strut(_selected.value(), hovered.value(), _connect_role,
                                 _connect_material);

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
            max_tension = _bridge.span_preview_capacity(length, _connect_role, _connect_material);
            show = true;
        }
    }

    if(show)
    {
        const bn::fixed load = _max_train_load();
        const bool weak = max_tension < load;
        const physics::StressBand band =
            weak ? physics::StressBand::red : physics::StressBand::green;
        const int frame = view::strut_frame_for_stress(_connect_role, band, true);
        view::position_strut_sprite(_preview_strut, from, to, frame);
        return;
    }

    _preview_strut.set_visible(false);
}

void Game::_update_build()
{
    bn::fixed speed = game_ui::CURSOR_SPEED;
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
        _weld_holding = true;
        _weld_dragged = false;
        _weld_press_pos = _cursor_pos;

        if(!_selected)
        {
            if(hovered)
            {
                _selected = hovered;
            }
            else
            {
                _handle_confirm(hovered, draw_at);
            }
        }
        else if(hovered && hovered.value() == _selected.value())
        {
            _selected.reset();
            _weld_holding = false;
        }
    }

    if(_weld_holding && bn::keypad::a_held())
    {
        const bn::fixed dx = _cursor_pos.x() - _weld_press_pos.x();
        const bn::fixed dy = _cursor_pos.y() - _weld_press_pos.y();

        if(dx * dx + dy * dy > bn::fixed(game_ui::WELD_DRAG_SQ))
        {
            _weld_dragged = true;
        }

        if(_weld_dragged)
        {
            _try_chain_weld(hovered, draw_at);
        }
    }

    if(_weld_holding && !bn::keypad::a_held())
    {
        const bool already_on_selected =
            hovered && _selected && hovered.value() == _selected.value();
        const bool connect = _weld_dragged || !hovered ||
            (_selected && hovered.value() != _selected.value());

        if(_selected && connect && !already_on_selected)
        {
            _handle_confirm(hovered, draw_at);
        }

        _weld_holding = false;
        _weld_dragged = false;
    }

    // B: DELETE — prefer beam under cursor, else player node (vision).
    if(bn::keypad::b_pressed())
    {
        const bn::optional<int> strut_hit = _bridge.find_strut_near(_cursor_pos);

        if(strut_hit && _bridge.remove_strut(strut_hit.value()))
        {
            _show_toast(game_ui::ToastKind::deleted);
        }
        else if(hovered && _bridge.is_player_node(hovered.value()))
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

    // L/R: CYCLE materials (vision legend).
    if(bn::keypad::l_pressed() && !bn::keypad::a_held())
    {
        _cycle_material(-1);
    }

    if(bn::keypad::r_pressed() && !bn::keypad::a_held() && !bn::keypad::left_held() &&
       !bn::keypad::right_held() && !bn::keypad::up_held() && !bn::keypad::down_held())
    {
        _cycle_material(1);
    }

    if(bn::keypad::select_pressed())
    {
        _enter_pause();
    }

    if(bn::keypad::start_pressed() && !bn::keypad::a_held())
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

        if(_bridge.budget_remaining() < bridge::NODE_COST)
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
            _bridge.preview_strut(_selected.value(), node_id, _connect_role, _connect_material);

        switch(preview.status)
        {
            case bridge::StrutPreviewStatus::duplicate:
                _show_toast(game_ui::ToastKind::dup);
                break;

            case bridge::StrutPreviewStatus::struts_full:
                _show_toast(game_ui::ToastKind::full);
                break;

            case bridge::StrutPreviewStatus::ok:
            {
                if(preview.max_tension < _max_train_load())
                {
                    _show_toast(game_ui::ToastKind::weak);
                }

                const int next_cost = bridge::link_cost(_connect_material) +
                                      (hovered ? 0 : bridge::NODE_COST);

                if(_bridge.budget_remaining() < next_cost)
                {
                    _show_toast(game_ui::ToastKind::full);
                    break;
                }

                (void)_bridge.add_strut(_selected.value(), node_id, _connect_role,
                                        _connect_material);
                bn::sound_items::ui.play();
                break;
            }

            default:
                break;
        }
    }

    _selected = node_id;
}

void Game::_try_chain_weld(bn::optional<int> hovered, bn::fixed_point draw_at)
{
    if(!_selected)
    {
        return;
    }

    const int from = _selected.value();

    if(hovered && hovered.value() == from)
    {
        return;
    }

    const bn::fixed_point from_pos = _bridge.nodes()[from].position;

    if(hovered)
    {
        const bridge::StrutPreview preview =
            _bridge.preview_strut(from, hovered.value(), _connect_role, _connect_material);

        if(preview.status == bridge::StrutPreviewStatus::ok)
        {
            _handle_confirm(hovered, draw_at);
            _weld_press_pos = _cursor_pos;
        }

        return;
    }

    bn::fixed_point to = game::snap_point(draw_at);
    const bn::fixed span2 = physics::squared_distance(from_pos, to);

    if(span2 < bn::fixed(game_ui::CHAIN_WELD_MIN_SQ))
    {
        return;
    }

    const bn::fixed span = physics::distance(from_pos, to);

    if(span >= game::CHAIN_WELD_MIN * 2)
    {
        const bn::fixed t = game::CHAIN_WELD_MIN.safe_division(span);
        to = game::snap_point(physics::lerp(from_pos, to, t));
    }

    if(to.x() == from_pos.x() && to.y() == from_pos.y())
    {
        return;
    }

    const bn::optional<int> near_joint = _bridge.find_node_near(to);

    if(near_joint && near_joint.value() == from)
    {
        return;
    }

    _handle_confirm(near_joint, to);
    _weld_press_pos = _cursor_pos;
}

void Game::_cycle_connect_role()
{
    if(!_bridge.allows_link_role(bridge::LinkRole::brace) &&
       !_bridge.allows_link_role(bridge::LinkRole::foundation))
    {
        _connect_role = bridge::LinkRole::track;
        _connect_material = bridge::MaterialId::wood;
        return;
    }

    _connect_role = _connect_role == bridge::LinkRole::track ?
        bridge::LinkRole::brace : bridge::LinkRole::track;
    _connect_material = bridge::material_for_role(_connect_role);
    bn::sound_items::ui.play();
}

void Game::_cycle_material(const int direction)
{
    bridge::MaterialId next = _connect_material;

    for(int i = 0; i < material::COUNT; ++i)
    {
        next = direction >= 0 ? material::next(next) : material::prev(next);

        if(_bridge.allows_material(next))
        {
            _connect_material = next;
            _connect_role = bridge::role_for_material(next);

            // Wood stays track; iron/cable prefer brace when allowed.
            if(next == bridge::MaterialId::wood)
            {
                _connect_role = bridge::LinkRole::track;
            }
            else if(next == bridge::MaterialId::iron || next == bridge::MaterialId::cable)
            {
                _connect_role = _bridge.allows_link_role(bridge::LinkRole::brace) ?
                    bridge::LinkRole::brace : bridge::LinkRole::track;
            }
            else if(next == bridge::MaterialId::stone)
            {
                _connect_role = bridge::LinkRole::foundation;
            }

            bn::sound_items::ui.play();
            return;
        }
    }
}

int Game::_build_tool_label() const
{
    switch(_connect_material)
    {
        case bridge::MaterialId::iron:
            return game_ui::LABEL_IRON;
        case bridge::MaterialId::cable:
            return game_ui::LABEL_CABLE;
        case bridge::MaterialId::stone:
            return game_ui::LABEL_STONE;
        default:
            return game_ui::LABEL_WOOD;
    }
}

void Game::_update_hud()
{
    const bool playing = _screen == Screen::playing;
    _top_bar.set_visible(playing);

    if(_no_go_timer > 0)
    {
        --_no_go_timer;
        _label.set_item(bn::sprite_items::label, game_ui::LABEL_NO_GO);
        _label.set_position(-88, game_ui::HUD_TOP_Y);
        _label.set_visible(true);
    }
    else if(_play_mode == PlayMode::test)
    {
        int status_frame = game_ui::LABEL_RUN;

        switch(_bridge.status())
        {
            case bridge::SimStatus::success:
                status_frame = game_ui::LABEL_WIN;
                break;

            case bridge::SimStatus::failed_fall:
                status_frame = game_ui::LABEL_FAIL;
                break;

            case bridge::SimStatus::no_path:
                status_frame = game_ui::LABEL_NO_GO;
                break;

            default:
                status_frame = game_ui::LABEL_RUN;
                break;
        }

        _label.set_item(bn::sprite_items::label, status_frame);
        _label.set_position(-88, game_ui::HUD_TOP_Y);
        _label.set_visible(true);

        // SIM: stress % (left digits) + STRESS tag via toast slot unused — strut digits = member count.
        const int stress_pct = (_bridge.peak_stress() * 100).integer();
        _set_counter(_node_count_digits, stress_pct, -20);
        _set_counter(_strut_count_digits, _bridge.struts().size(), 40);
        _hint_label.set_item(bn::sprite_items::label, game_ui::LABEL_STRESS);
        _hint_label.set_position(-56, game_ui::HUD_TOP_Y);
        _hint_label.set_visible(true);
        _hint_secondary.set_visible(false);
        _panel.set_visible(false);
    }
    else if(_play_mode == PlayMode::build)
    {
        _label.set_item(bn::sprite_items::label, game_ui::LABEL_EDIT);
        _label.set_position(-88, game_ui::HUD_TOP_Y);
        _label.set_visible(true);

        // EDIT: 4-digit £ remaining across both digit pairs (AB CD).
        const int remaining = _bridge.budget_remaining();
        const int hi = remaining / 100;
        const int lo = remaining % 100;
        _set_counter(_node_count_digits, hi, -28);
        _set_counter(_strut_count_digits, lo, -8);

        _panel.set_visible(true);
        _panel.set_position(0, game_ui::HUD_BOTTOM_Y - 2);
        _hint_label.set_item(bn::sprite_items::label, _build_tool_label());
        _hint_label.set_position(-48, game_ui::HUD_BOTTOM_Y);
        _hint_label.set_visible(true);

        if(_bridge.has_track_path())
        {
            _hint_secondary.set_item(bn::sprite_items::label, game_ui::LABEL_START);
        }
        else
        {
            _hint_secondary.set_item(bn::sprite_items::label, game_ui::LABEL_A_DRAW);
        }

        _hint_secondary.set_position(48, game_ui::HUD_BOTTOM_Y);
        _hint_secondary.set_visible(true);
    }

    _set_weight_digits(_max_train_load().integer(), playing);
}

void Game::_update_build_hints()
{
    // Vision bottom strip is driven by `_update_hud` (material + A DRAW / START).
    // Keep the first-time onboarding cycle on the secondary line only.
    if(_screen != Screen::playing || _play_mode != PlayMode::build)
    {
        return;
    }

    if(!_build_hints_seen)
    {
        ++_build_hint_timer;

        if(_build_hint_timer >= game_ui::BUILD_HINT_FRAMES || bn::keypad::any_pressed())
        {
            _build_hints_seen = true;
            return;
        }

        const int phase = (_build_hint_timer / 60) % 3;
        int frame = game_ui::LABEL_A_DRAW;

        if(phase == 1)
        {
            frame = game_ui::LABEL_B_DEL;
        }
        else if(phase == 2)
        {
            frame = game_ui::LABEL_LR_CY;
        }

        _hint_secondary.set_visible(true);
        _hint_secondary.set_position(48, game_ui::HUD_BOTTOM_Y);
        _hint_secondary.set_item(bn::sprite_items::label, frame);
    }
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
    _weld_holding = false;
    _weld_dragged = false;
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
