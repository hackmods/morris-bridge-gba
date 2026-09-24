#ifndef MORRIS_BRIDGE_GAME_H
#define MORRIS_BRIDGE_GAME_H

/**
 * @file
 * GBA game orchestrator — delegates per-screen logic to src/screens/*.
 */

#include "bn_camera_ptr.h"
#include "bn_optional.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "audio_director.h"
#include "bg_view.h"
#include "bridge_system.h"
#include "engine.h"
#include "game_ui.h"
#include "graph_view.h"
#include "progress.h"
#include "run_score.h"
#include "screen_manager.h"
#include "screen_types.h"

class Game
{
public:
    Game();
    void update();

private:
    using Screen = screen::Id;
    using PlayMode = screen::PlayMode;
    using PauseOption = screen::PauseOption;

    void _register_screen_handlers();
    void _sync_graph_sprites();

    [[nodiscard]] int _lore_roll(int salt) const;
    [[nodiscard]] bool _lore_rare(int salt) const;
    [[nodiscard]] int _pick_win_label() const;
    [[nodiscard]] int _pick_fail_label() const;
    [[nodiscard]] int _pick_win_banner() const;
    [[nodiscard]] int _pick_fail_banner() const;
    [[nodiscard]] bn::fixed _max_train_load() const;

    void _load_progress();
    void _persist_progress();
    void _trigger_shake();
    void _update_shake();
    void _spawn_dust(bn::fixed_point origin);
    void _update_dust();
    void _update_parallax();
    void _animate_backdrop();
    [[nodiscard]] engine::Id _active_engine() const;
    void _sync_train_sprite();
    void _set_weight_digits(int value, bool visible);
    void _step_level(int delta);
    void _sync_level_select_flavor();
    void _sync_level_select_stars();
    void _sync_level_select_card();
    void _step_engine(int delta);
    bool _dir_repeat(bool pressed, bool held, int& counter);
    void _place_splash_tiles();
    void _set_splash_visible(bool visible);

    void _show_title();
    void _show_level_select();
    void _show_engine_select();
    void _begin_pending_level();
    void _show_level_intro(int level_index);
    void _start_level(int level_index);
    void _show_gameplay();
    void _hide_gameplay();
    void _enter_pause();
    void _leave_pause();
    void _show_level_complete();
    void _show_level_failed();
    void _show_all_complete();

    void _update_title();
    void _update_level_select();
    void _update_engine_select();
    void _update_level_intro();
    void _update_pause();
    void _update_level_complete();
    void _update_level_failed();
    void _update_all_complete();
    void _update_playing();

    void _set_counter(bn::vector<bn::sprite_ptr, 2>& digits, int value, bn::fixed x_base);
    void _show_toast(game_ui::ToastKind kind);
    void _update_toast();
    void _update_preview();
    void _update_build_hints();
    void _update_build();
    void _handle_confirm(bn::optional<int> hovered, bn::fixed_point place_at);
    void _try_chain_weld(bn::optional<int> hovered, bn::fixed_point draw_at);
    void _cycle_connect_role();
    void _cycle_material(int direction);
    [[nodiscard]] int _build_tool_label() const;
    void _enter_test();
    void _leave_test();
    void _update_test();
    void _update_hud();
    void _animate_train();

    progress::Progress _progress;
    engine::Id _selected_engine = engine::default_engine();
    engine::Id _newly_unlocked = engine::none;
    int _pending_level = 0;
    bool _show_unlock_flash = false;
    int _shake_timer = 0;
    bn::fixed _parallax_shift = 0;
    bridge::BridgeSystem _bridge;
    Screen _screen = Screen::title;
    PlayMode _play_mode = PlayMode::build;
    screen::Manager _screen_manager;
    bridge::LinkRole _connect_role = bridge::LinkRole::track;
    bridge::MaterialId _connect_material = bridge::MaterialId::wood;
    bn::fixed_point _cursor_pos;
    bn::optional<int> _selected;
    int _selected_level = 0;
    PauseOption _pause_option = PauseOption::resume;
    audio::Director _audio;
    int _frame = 0;
    int _menu_up_counter = 0;
    int _menu_down_counter = 0;
    bool _train_facing_right = true;
    bn::fixed _train_prev_x = 0;
    int _chuff_cooldown = 0;
    int _last_chuff_frame = 1;
    int _toast_timer = 0;
    game_ui::ToastKind _toast_kind = game_ui::ToastKind::none;
    bool _build_hints_seen = false;
    int _build_hint_timer = 0;
    int _no_go_timer = 0;
    int _fail_reveal_timer = 0;
    bool _weld_holding = false;
    bool _weld_dragged = false;
    bn::fixed_point _weld_press_pos;
    session::RunScore _last_run_score;

    bn::sprite_ptr _cursor;
    bn::sprite_ptr _train;
    bn::sprite_ptr _label;
    bn::sprite_ptr _hint_label;
    bn::sprite_ptr _hint_secondary;
    bn::sprite_ptr _panel;
    bn::sprite_ptr _top_bar;
    bn::sprite_ptr _banner;
    bn::sprite_ptr _logo;
    bn::sprite_ptr _menu_row;
    bn::sprite_ptr _menu_arrow;
    bn::sprite_ptr _track_card;
    bn::sprite_ptr _hatt;
    bn::sprite_ptr _ghost;
    bn::sprite_ptr _ring;
    bn::sprite_ptr _preview_strut;
    bn::sprite_ptr _toast;
    bn::sprite_ptr _portrait;
    bn::camera_ptr _camera;
    bn::vector<game_ui::DustPuff, game_ui::MAX_DUST> _dust;
    bn::vector<bn::sprite_ptr, 2> _node_count_digits;
    bn::vector<bn::sprite_ptr, 2> _strut_count_digits;
    bn::vector<bn::sprite_ptr, 2> _weight_digits;
    view::BackdropView _backdrop;
    view::GraphView _graph;
    bn::vector<bn::sprite_ptr, 4> _splash;

    friend void screen_trampoline_title(void* context);
    friend void screen_trampoline_level_select(void* context);
    friend void screen_trampoline_engine_select(void* context);
    friend void screen_trampoline_level_intro(void* context);
    friend void screen_trampoline_playing(void* context);
    friend void screen_trampoline_pause(void* context);
    friend void screen_trampoline_level_complete(void* context);
    friend void screen_trampoline_level_failed(void* context);
    friend void screen_trampoline_all_complete(void* context);
};

#endif
