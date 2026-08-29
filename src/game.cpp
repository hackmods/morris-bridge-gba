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


Game::Game() :
    _cursor(bn::sprite_items::cursor.create_sprite(0, 0)),
    _train(bn::sprite_items::train.create_sprite(_bridge.train_position())),
    _label(bn::sprite_items::label.create_sprite(0, 66)),
    _hint_label(bn::sprite_items::label.create_sprite(0, 44)),
    _hint_secondary(bn::sprite_items::label.create_sprite(0, 54)),
    _panel(bn::sprite_items::panel.create_sprite(0, 66)),
    _banner(bn::sprite_items::banner.create_sprite(0, -36)),
    _logo(bn::sprite_items::logo.create_sprite(0, -52)),
    _menu_row(bn::sprite_items::menu.create_sprite(0, 8)),
    _menu_arrow(bn::sprite_items::cursor.create_sprite(-36, 8)),
    _hatt(bn::sprite_items::hatt.create_sprite(52, 8)),
    _ghost(game_ui::make_sprite(bn::sprite_items::ghost, 0, 0, game_ui::Z_GHOST)),
    _ring(game_ui::make_sprite(bn::sprite_items::ring, 0, 0, game_ui::Z_RING)),
    _preview_strut(game_ui::make_sprite(bn::sprite_items::strut, 0, 0, game_ui::Z_STRUT_PREVIEW)),
    _toast(game_ui::make_sprite(bn::sprite_items::toast, 0, 52, game_ui::Z_TOAST)),
    _portrait(bn::sprite_items::portrait.create_sprite(56, 44)),
    _camera(bn::camera_ptr::create())
{
    bn::bg_palettes::set_transparent_color(bn::color(8, 14, 20));

    _cursor.set_z_order(game_ui::Z_CURSOR);
    _train.set_z_order(-15);
    _train.set_visible(false);
    _label.set_z_order(-18);
    _hint_label.set_z_order(-18);
    _hint_secondary.set_z_order(-18);
    _hint_label.set_visible(false);
    _hint_secondary.set_visible(false);
    _panel.set_z_order(-19);
    _banner.set_z_order(-30);
    _logo.set_z_order(-29);
    _logo.set_visible(false);
    _menu_row.set_z_order(-25);
    _menu_arrow.set_z_order(-24);
    _hatt.set_z_order(-23);
    _ghost.set_visible(false);
    _ring.set_visible(false);
    _preview_strut.set_visible(false);
    _toast.set_visible(false);
    _portrait.set_z_order(-22);
    _portrait.set_visible(false);

    _node_count_digits.push_back(game_ui::make_sprite(bn::sprite_items::digit, -20, 66, game_ui::Z_DIGIT));
    _node_count_digits.push_back(game_ui::make_sprite(bn::sprite_items::digit, -12, 66, game_ui::Z_DIGIT));
    _strut_count_digits.push_back(game_ui::make_sprite(bn::sprite_items::digit, 12, 66, game_ui::Z_DIGIT));
    _strut_count_digits.push_back(game_ui::make_sprite(bn::sprite_items::digit, 20, 66, game_ui::Z_DIGIT));
    _weight_digits.push_back(game_ui::make_sprite(bn::sprite_items::digit, -4, 66, game_ui::Z_DIGIT));
    _weight_digits.push_back(game_ui::make_sprite(bn::sprite_items::digit, 4, 66, game_ui::Z_DIGIT));

    _load_progress();
    _place_splash_tiles();
    _backdrop.init();
    _register_screen_handlers();
    _show_title();
}

void Game::update()
{
    ++_frame;
    _animate_backdrop();
    _update_shake();
    _update_dust();
    _screen_manager.update(_screen, this);
    _animate_train();
}

void Game::_register_screen_handlers()
{
    _screen_manager.set_handler(Screen::title, screen_trampoline_title);
    _screen_manager.set_handler(Screen::level_select, screen_trampoline_level_select);
    _screen_manager.set_handler(Screen::engine_select, screen_trampoline_engine_select);
    _screen_manager.set_handler(Screen::level_intro, screen_trampoline_level_intro);
    _screen_manager.set_handler(Screen::playing, screen_trampoline_playing);
    _screen_manager.set_handler(Screen::pause, screen_trampoline_pause);
    _screen_manager.set_handler(Screen::level_complete, screen_trampoline_level_complete);
    _screen_manager.set_handler(Screen::level_failed, screen_trampoline_level_failed);
    _screen_manager.set_handler(Screen::all_complete, screen_trampoline_all_complete);
}

void Game::_sync_graph_sprites()
{
    const bool visible = _screen == Screen::playing;
    const bn::optional<int> selected =
        visible && _play_mode == PlayMode::build ? _selected : bn::optional<int>();

    _graph.sync_nodes(_bridge, visible, _frame, selected, _ring);
    _graph.sync_struts(_bridge, visible, _play_mode == PlayMode::test, _frame);
}

int Game::_lore_roll(int salt) const
{
    return (_frame + _selected_level * 17 + _bridge.current_level() * 31 + salt * 13) % 100;
}

bool Game::_lore_rare(int salt) const
{
    return _lore_roll(salt) < 4;
}

int Game::_pick_win_label() const
{
    if(! _lore_rare(1))
    {
        return game_ui::LABEL_PEEP;
    }

    return (_lore_roll(2) & 1) ? game_ui::LABEL_CHUFF : game_ui::LABEL_TOOT;
}

int Game::_pick_fail_label() const
{
    if(! _lore_rare(3))
    {
        return game_ui::LABEL_ASHES;
    }

    switch(_lore_roll(4) % 3)
    {
        case 0:
            return game_ui::LABEL_OOPS;

        case 1:
            return game_ui::LABEL_WAIT;

        default:
            return game_ui::LABEL_AGAIN;
    }
}

int Game::_pick_win_banner() const
{
    return _lore_rare(5) ? game_ui::BANNER_HATT_PEEP : game_ui::BANNER_HATT_OK;
}

int Game::_pick_fail_banner() const
{
    return _lore_rare(6) ? game_ui::BANNER_DELAY_LISTEN : game_ui::BANNER_DELAY;
}

bn::fixed Game::_max_train_load() const
{
    return _bridge.effective_weight_at(bn::fixed(256), bn::fixed(256));
}

void Game::_load_progress()
{
    std::uint8_t buffer[save_data::SIZE];
    bn::span<uint8_t> span(buffer, save_data::SIZE);
    bn::sram::read_span(span);
    (void)save_data::decode(_progress, buffer);
}

void Game::_persist_progress()
{
    std::uint8_t buffer[save_data::SIZE];
    (void)save_data::encode(_progress, buffer);
    bn::span<const uint8_t> span(buffer, save_data::SIZE);
    bn::sram::write_span(span);
}

void Game::_trigger_shake()
{
    _shake_timer = game_ui::SHAKE_FRAMES;
}

void Game::_update_shake()
{
    if(_shake_timer > 0)
    {
        --_shake_timer;
        _camera.set_x((_shake_timer % 2 == 0) ? bn::fixed(2) : bn::fixed(-2));
        return;
    }

    _camera.set_x(0);
}

void Game::_spawn_dust(bn::fixed_point origin)
{
    for(int i = 0; i < 4; ++i)
    {
        if(_dust.full())
        {
            break;
        }

        _dust.push_back(game_ui::DustPuff{
            game_ui::make_sprite(bn::sprite_items::ghost,
                        origin.x() + bn::fixed(i * 4 - 6), origin.y(), 3),
            bn::fixed(-1) / 4,
            game_ui::DUST_LIFE,
        });
    }
}

void Game::_update_dust()
{
    for(int i = _dust.size() - 1; i >= 0; --i)
    {
        DustPuff& puff = _dust[i];

        if(puff.life <= 0)
        {
            _dust.erase(_dust.begin() + i);
            continue;
        }

        --puff.life;
        bn::fixed_point pos = puff.sprite.position();
        pos.set_y(pos.y() + puff.vy);
        puff.vy += bn::fixed(1) / 16;
        puff.sprite.set_position(pos);
        puff.sprite.set_visible(_screen == Screen::playing || _screen == Screen::level_failed);
    }
}

void Game::_update_parallax()
{
    bn::fixed shift = 0;

    if(_screen == Screen::playing && _play_mode == PlayMode::test)
    {
        const level::Def& def = _bridge.level_def();
        shift = _bridge.train_position().x().safe_multiplication(def.parallax_front);
    }
    else if(_screen == Screen::playing || _screen == Screen::engine_select ||
            _screen == Screen::level_select)
    {
        // Gentle idle drift so build screens do not look frozen.
        shift = bn::fixed(_frame).safe_division(512);
    }

    _parallax_shift = shift;
}

void Game::_animate_backdrop()
{
    _update_parallax();
    _backdrop.animate_clouds();

    if(_screen == Screen::playing || _screen == Screen::engine_select ||
       _screen == Screen::level_select || _screen == Screen::level_complete ||
       _screen == Screen::level_failed)
    {
        _backdrop.apply_parallax(_parallax_shift, _bridge.level_def());
    }
}

engine::Id Game::_active_engine() const
{
    if(_screen == Screen::level_complete && _show_unlock_flash &&
       engine::is_playable(_newly_unlocked))
    {
        return _newly_unlocked;
    }

    if(_screen == Screen::playing || _screen == Screen::pause ||
       _screen == Screen::level_complete || _screen == Screen::level_failed)
    {
        return _bridge.engine();
    }

    return _selected_engine;
}

void Game::_sync_train_sprite()
{
    if(! _train.visible())
    {
        return;
    }

    const engine::Id id = _active_engine();
    int chuff = (_frame / 10) % engine::FRAMES_PER_ENGINE;

    if(_screen == Screen::playing && _play_mode == PlayMode::test &&
       _bridge.status() == bridge::SimStatus::running)
    {
        const bn::fixed speed = _bridge.current_train_speed();
        const int speed_i = speed.shift_integer();
        const int speed_scale = speed_i >= 5 ? 1 : (5 - speed_i);
        const int motion = _bridge.train_path_index() * 64 +
                           _bridge.train_link_t().shift_integer() * 8 +
                           _frame / speed_scale;
        chuff = motion % engine::FRAMES_PER_ENGINE;
    }

    _train.set_item(bn::sprite_items::train, engine::train_frame_base(id) + chuff);
}

void Game::_set_weight_digits(int value, bool visible)
{
    const int clamped = value < 0 ? 0 : (value > 99 ? 99 : value);
    _weight_digits[0].set_item(bn::sprite_items::digit, clamped / 10);
    _weight_digits[0].set_position(-4, 66);
    _weight_digits[0].set_visible(visible && clamped >= 10);
    _weight_digits[1].set_item(bn::sprite_items::digit, clamped % 10);
    _weight_digits[1].set_position(clamped >= 10 ? 4 : 0, 66);
    _weight_digits[1].set_visible(visible);
}

void Game::_step_level(const int delta)
{
    int next = _selected_level;

    for(int attempt = 0; attempt < bridge::LEVEL_COUNT; ++attempt)
    {
        next = (next + delta + bridge::LEVEL_COUNT) % bridge::LEVEL_COUNT;

        if(_progress.is_level_unlocked(next))
        {
            _selected_level = next;
            break;
        }
    }

    _menu_row.set_item(bn::sprite_items::menu, _selected_level);
    _sync_train_sprite();
    _sync_level_select_stars();
    _sync_level_select_whisper();
}

void Game::_step_engine(const int delta)
{
    int index = int(_selected_engine);

    for(int attempt = 0; attempt < engine::engine_count(); ++attempt)
    {
        index = (index + delta + engine::engine_count()) % engine::engine_count();
        const engine::Id candidate = static_cast<engine::Id>(index);

        if(_progress.is_engine_unlocked(candidate))
        {
            _selected_engine = candidate;
            break;
        }
    }

    const engine::Profile& profile = engine::profile(_selected_engine);
    _menu_row.set_item(bn::sprite_items::menu, engine::menu_frame(_selected_engine));
    _set_weight_digits(profile.weight.integer(), true);
    _sync_train_sprite();
}

bool Game::_dir_repeat(bool pressed, bool held, int& counter)
{
    if(pressed)
    {
        counter = 0;
        return true;
    }

    if(held)
    {
        ++counter;

        if(counter > game_ui::CURSOR_REPEAT_DELAY &&
           (counter - game_ui::CURSOR_REPEAT_DELAY) % game_ui::CURSOR_REPEAT_RATE == 0)
        {
            return true;
        }

        return false;
    }

    counter = 0;
    return false;
}

void Game::_place_splash_tiles()
{
    const int frames[] = { 0, 1, 2, 3 };
    const int xs[] = { -32, 32, -32, 32 };
    const int ys[] = { -32, -32, 32, 32 };

    for(int i = 0; i < 4; ++i)
    {
        bn::sprite_ptr tile = bn::sprite_items::splash.create_sprite(xs[i], ys[i]);
        tile.set_item(bn::sprite_items::splash, frames[i]);
        tile.set_z_order(-28);
        tile.set_visible(false);
        _splash.push_back(tile);
    }
}

void Game::_set_splash_visible(bool visible)
{
    for(bn::sprite_ptr& tile : _splash)
    {
        tile.set_visible(visible);
    }

    if(! visible)
    {
        _logo.set_visible(false);
    }
}

void Game::_begin_pending_level()
{
    const level::Def& def = level::get(_pending_level);

    if(def.intro_dialog_id > 0 && ! _progress.is_intro_seen(_pending_level))
    {
        _show_level_intro(_pending_level);
        return;
    }

    _start_level(_pending_level);
}

void Game::_start_level(int level_index)
{
    _selected_level = level_index;
    _bridge.load_level(level_index);
    _bridge.set_engine(_selected_engine);
    _screen = Screen::playing;
    _play_mode = PlayMode::build;
    _selected.reset();
    _connect_role = bridge::LinkRole::track;
    _pause_option = PauseOption::resume;
    _build_hint_timer = 0;
    _cursor_pos = _bridge.nodes()[bridge::START_NODE].position;
    _cursor_pos.set_x(_cursor_pos.x() + 16);
    _backdrop.layout_for_level(_bridge.level_def());
    _backdrop.apply_parallax(_parallax_shift, _bridge.level_def());
    _show_gameplay();
    _sync_graph_sprites();
    _audio.play(audio::MusicCue::workshop);
}

void Game::_hide_gameplay()
{
    _cursor.set_visible(false);
    _train.set_visible(false);
    _backdrop.set_scenery_visible(false);
    _graph.sync_nodes(_bridge, false, _frame, bn::nullopt, _ring);
    _graph.sync_struts(_bridge, false, false, _frame);
}

void Game::_enter_pause()
{
    _screen = Screen::pause;
    _set_splash_visible(false);
    _pause_option = PauseOption::resume;
    _banner.set_visible(true);
    _banner.set_item(bn::sprite_items::banner, game_ui::BANNER_LEVEL_SELECT);
    _banner.set_position(0, -48);
    _panel.set_visible(true);
    _panel.set_item(bn::sprite_items::panel, game_ui::PANEL_SELECT);
    _panel.set_position(0, 12);
    _label.set_visible(false);
    _hint_label.set_visible(false);
    _hint_secondary.set_visible(false);
    _menu_row.set_visible(true);
    _menu_row.set_item(bn::sprite_items::menu, game_ui::MENU_PAUSE_BASE + int(_pause_option));
    _menu_row.set_position(0, 0);
    _menu_arrow.set_visible(true);
    _menu_arrow.set_position(-38, 8);
    _cursor.set_visible(false);
    _hatt.set_visible(true);
    _hatt.set_position(56, 8);
    _hatt.set_item(bn::sprite_items::hatt, game_ui::HATT_FURIOUS);

    if(bn::music::playing())
    {
        _audio.duck_for_pause();
    }
}

void Game::_leave_pause()
{
    _screen = Screen::playing;
    _banner.set_visible(false);
    _menu_row.set_visible(false);
    _menu_arrow.set_visible(false);
    _hatt.set_visible(false);
    _panel.set_visible(true);
    _panel.set_item(bn::sprite_items::panel, game_ui::PANEL_HUD);
    _panel.set_position(0, 66);
    _label.set_visible(true);
    _label.set_position(0, 68);
    _label.set_item(bn::sprite_items::label, game_ui::LABEL_BUILD);
    _cursor.set_visible(true);

    if(bn::music::playing())
    {
        _audio.restore_from_pause();
    }
}

void Game::_animate_train()
{
    const int chuff_frame = (_frame / game_ui::CHUFF_FRAMES) % 2;
    _sync_train_sprite();

    if(_play_mode == PlayMode::test &&
       _bridge.status() == bridge::SimStatus::running &&
       chuff_frame == 0 &&
       _last_chuff_frame != 0 &&
       _chuff_cooldown <= 0)
    {
        bn::sound_items::chuff.play();
        _chuff_cooldown = game_ui::CHUFF_FRAMES / 2;
    }

    _last_chuff_frame = chuff_frame;

    if(_chuff_cooldown > 0)
    {
        --_chuff_cooldown;
    }
}

void screen_trampoline_title(void* context)
{
    static_cast<Game*>(context)->_update_title();
}

void screen_trampoline_level_select(void* context)
{
    static_cast<Game*>(context)->_update_level_select();
}

void screen_trampoline_engine_select(void* context)
{
    static_cast<Game*>(context)->_update_engine_select();
}

void screen_trampoline_level_intro(void* context)
{
    static_cast<Game*>(context)->_update_level_intro();
}

void screen_trampoline_playing(void* context)
{
    static_cast<Game*>(context)->_update_playing();
}

void screen_trampoline_pause(void* context)
{
    static_cast<Game*>(context)->_update_pause();
}

void screen_trampoline_level_complete(void* context)
{
    static_cast<Game*>(context)->_update_level_complete();
}

void screen_trampoline_level_failed(void* context)
{
    static_cast<Game*>(context)->_update_level_failed();
}

void screen_trampoline_all_complete(void* context)
{
    static_cast<Game*>(context)->_update_all_complete();
}

