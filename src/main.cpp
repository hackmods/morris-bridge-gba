#include "bn_core.h"
#include "bn_array.h"
#include "bn_color.h"
#include "bn_bg_palettes.h"
#include "bn_camera_ptr.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_music.h"
#include "bn_optional.h"
#include "bn_sound_items.h"
#include "bn_sprite_builder.h"
#include "bn_sprite_ptr.h"
#include "bn_sram.h"
#include "bn_vector.h"

#include "bn_music_items_full_steam.h"
#include "bn_music_items_island_line.h"
#include "bn_music_items_workshop.h"
#include "bn_music_items_yard.h"
#include "bn_sound_items_chuff.h"
#include "bn_sound_items_delay.h"
#include "bn_sound_items_peep.h"
#include "bn_sound_items_snap.h"
#include "bn_sound_items_splash.h"
#include "bn_sound_items_ui.h"
#include "bn_sound_items_useful.h"
#include "bn_sound_items_whistle.h"

#include "bn_sprite_items_anchor.h"
#include "bn_sprite_items_banner.h"
#include "bn_sprite_items_cloud.h"
#include "bn_sprite_items_cursor.h"
#include "bn_sprite_items_digit.h"
#include "bn_sprite_items_ghost.h"
#include "bn_sprite_items_ground.h"
#include "bn_sprite_items_hatt.h"
#include "bn_sprite_items_label.h"
#include "bn_sprite_items_logo.h"
#include "bn_sprite_items_menu.h"
#include "bn_sprite_items_node.h"
#include "bn_sprite_items_panel.h"
#include "bn_sprite_items_portrait.h"
#include "bn_sprite_items_ring.h"
#include "bn_sprite_items_scenery_far.h"
#include "bn_sprite_items_splash.h"
#include "bn_sprite_items_sky.h"
#include "bn_sprite_items_strut.h"
#include "bn_sprite_items_toast.h"
#include "bn_sprite_items_train.h"
#include "bn_sprite_items_water.h"

#include "bridge_system.h"
#include "engine.h"
#include "game_constants.h"
#include "level.h"
#include "physics.h"
#include "progress.h"
#include "save_data.h"

namespace
{

constexpr bn::fixed CURSOR_SPEED = 3;
constexpr bn::fixed CURSOR_FAST = 6;
constexpr bn::fixed STRUT_SPRITE_WIDTH = 16;
constexpr int CURSOR_REPEAT_DELAY = 10;
constexpr int CURSOR_REPEAT_RATE = 3;
constexpr int TOAST_FRAMES = 90;
constexpr int SHAKE_FRAMES = 10;
constexpr int DUST_LIFE = 24;
constexpr int MAX_DUST = 6;
constexpr int CHUFF_FRAMES = 10;
constexpr bn::fixed MUSIC_VOLUME = bn::fixed(1) / 2;
constexpr bn::fixed MUSIC_PAUSE_VOLUME = bn::fixed(1) / 5;

constexpr int TOAST_DUP = 0;
constexpr int TOAST_WEAK = 1;
constexpr int TOAST_DEL = 2;

constexpr int STRUT_IDLE = 0;
constexpr int STRUT_HOT = 1;
constexpr int STRUT_PREVIEW_OK = 2;
constexpr int STRUT_PREVIEW_WEAK = 3;

constexpr int Z_CURSOR = -32767;
constexpr int Z_TOAST = -17;
constexpr int Z_DIGIT = -19;
constexpr int Z_RING = 4;
constexpr int Z_NODE = 5;
constexpr int Z_GHOST = 6;
constexpr int Z_STRUT_PREVIEW = 9;
constexpr int Z_STRUT = 10;

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

bn::sprite_ptr make_sprite(const bn::sprite_item& item, bn::fixed x, bn::fixed y, int z_order)
{
    bn::sprite_builder builder(item);
    builder.set_position(x, y);
    builder.set_z_order(z_order);
    return builder.release_build();
}

bn::fixed clamp(bn::fixed value, bn::fixed lo, bn::fixed hi)
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

void set_train_sprite_position(bn::sprite_ptr& train, bn::fixed_point track_position)
{
    train.set_position(game::train_display_position(track_position));
}

enum class MusicCue
{
    none,
    island_line,
    yard,
    workshop,
    full_steam,
};

void play_music(MusicCue cue, MusicCue& current)
{
    if(cue == current)
    {
        return;
    }

    current = cue;

    switch(cue)
    {
        case MusicCue::none:
            bn::music::stop();
            break;

        case MusicCue::island_line:
            bn::music_items::island_line.play(MUSIC_VOLUME, true);
            break;

        case MusicCue::yard:
            bn::music_items::yard.play(MUSIC_VOLUME, true);
            break;

        case MusicCue::workshop:
            bn::music_items::workshop.play(MUSIC_VOLUME, true);
            break;

        case MusicCue::full_steam:
            bn::music_items::full_steam.play(MUSIC_VOLUME, true);
            break;
    }
}

constexpr int LABEL_BUILD = 0;
constexpr int LABEL_TEST = 1;
constexpr int LABEL_DELAY = 2;
constexpr int LABEL_USEFUL = 3;
constexpr int LABEL_NO_GO = 4;
constexpr int LABEL_PEEP = 5;
constexpr int LABEL_ASHES = 6;
constexpr int LABEL_START = 7;
constexpr int LABEL_HINT_NODE = 8;
constexpr int LABEL_HINT_TEST = 9;
constexpr int LABEL_HINT_PAUSE = 10;

constexpr int BUILD_HINT_FRAMES = 240;

constexpr int LABEL_CHUFF = 11;
constexpr int LABEL_TOOT = 12;
constexpr int LABEL_OOPS = 13;
constexpr int LABEL_WAIT = 14;
constexpr int LABEL_AGAIN = 15;
constexpr int LABEL_NEW = 16;

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

bn::fixed strut_angle_deg(bn::fixed_point a, bn::fixed_point b)
{
    const bn::fixed dx = b.x() - a.x();
    const bn::fixed dy = b.y() - a.y();
    const int yi = (dy * 16).integer();
    const int xi = (dx * 16).integer();
    bn::fixed angle = bn::degrees_diamond_angle(yi, xi);

    if(angle < 0)
    {
        angle += 360;
    }

    return angle;
}

void position_strut_sprite(bn::sprite_ptr& sprite, bn::fixed_point a, bn::fixed_point b, int frame)
{
    const bn::fixed span = physics::distance(a, b);

    if(span <= 0)
    {
        sprite.set_visible(false);
        return;
    }

    const bn::fixed_point mid((a.x() + b.x()) / 2, (a.y() + b.y()) / 2);
    const bn::fixed scale = span.safe_division(STRUT_SPRITE_WIDTH);

    sprite.set_position(mid);
    sprite.set_item(bn::sprite_items::strut, frame);
    sprite.set_horizontal_scale(scale);
    sprite.set_vertical_scale(bn::fixed(1));
    sprite.set_rotation_angle_safe(strut_angle_deg(a, b));
    sprite.set_visible(true);
}

class Game
{
public:
    Game() :
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
        _ghost(make_sprite(bn::sprite_items::ghost, 0, 0, Z_GHOST)),
        _ring(make_sprite(bn::sprite_items::ring, 0, 0, Z_RING)),
        _preview_strut(make_sprite(bn::sprite_items::strut, 0, 0, Z_STRUT_PREVIEW)),
        _toast(make_sprite(bn::sprite_items::toast, 0, 52, Z_TOAST)),
        _portrait(bn::sprite_items::portrait.create_sprite(56, 44)),
        _camera(bn::camera.create())
    {
        bn::bg_palettes::set_transparent_color(bn::color(8, 14, 20));

        _camera.set_z_order(0);

        _cursor.set_z_order(Z_CURSOR);
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

        _node_count_digits.push_back(make_sprite(bn::sprite_items::digit, -20, 66, Z_DIGIT));
        _node_count_digits.push_back(make_sprite(bn::sprite_items::digit, -12, 66, Z_DIGIT));
        _strut_count_digits.push_back(make_sprite(bn::sprite_items::digit, 12, 66, Z_DIGIT));
        _strut_count_digits.push_back(make_sprite(bn::sprite_items::digit, 20, 66, Z_DIGIT));
        _weight_digits.push_back(make_sprite(bn::sprite_items::digit, -4, 66, Z_DIGIT));
        _weight_digits.push_back(make_sprite(bn::sprite_items::digit, 4, 66, Z_DIGIT));

        _load_progress();
        _place_splash_tiles();
        _place_backdrop();
        _place_scenery();
        _show_title();
    }

    void update()
    {
        ++_frame;
        _animate_backdrop();
        _update_shake();
        _update_dust();

        switch(_screen)
        {
            case Screen::title:
                _update_title();
                break;

            case Screen::level_select:
                _update_level_select();
                break;

            case Screen::engine_select:
                _update_engine_select();
                break;

            case Screen::playing:
                _update_playing();
                break;

            case Screen::pause:
                _update_pause();
                break;

            case Screen::level_complete:
                _update_level_complete();
                break;

            case Screen::level_failed:
                _update_level_failed();
                break;

            case Screen::all_complete:
                _update_all_complete();
                break;
        }

        _animate_train();
    }

private:
    enum class Screen
    {
        title,
        level_select,
        engine_select,
        playing,
        pause,
        level_complete,
        level_failed,
        all_complete,
    };

    enum class PlayMode
    {
        build,
        test,
    };

    enum class PauseOption
    {
        resume = 0,
        restart,
        levels,
        title,
        count,
    };

    int _lore_roll(int salt) const
    {
        return (_frame + _selected_level * 17 + _bridge.current_level() * 31 + salt * 13) % 100;
    }

    bool _lore_rare(int salt) const
    {
        return _lore_roll(salt) < 4;
    }

    int _pick_win_label() const
    {
        if(! _lore_rare(1))
        {
            return LABEL_PEEP;
        }

        return (_lore_roll(2) & 1) ? LABEL_CHUFF : LABEL_TOOT;
    }

    int _pick_fail_label() const
    {
        if(! _lore_rare(3))
        {
            return LABEL_ASHES;
        }

        switch(_lore_roll(4) % 3)
        {
            case 0:
                return LABEL_OOPS;

            case 1:
                return LABEL_WAIT;

            default:
                return LABEL_AGAIN;
        }
    }

    int _pick_win_banner() const
    {
        return _lore_rare(5) ? BANNER_HATT_PEEP : BANNER_HATT_OK;
    }

    int _pick_fail_banner() const
    {
        return _lore_rare(6) ? BANNER_DELAY_LISTEN : BANNER_DELAY;
    }

    [[nodiscard]] bn::fixed _max_train_load() const
    {
        return _bridge.effective_weight_at(bn::fixed(256), bn::fixed(256));
    }

    void _load_progress()
    {
        bn::sram sram;
        bn::array<std::uint8_t, save_data::SIZE> buffer;

        if(sram.read(buffer))
        {
            (void)save_data::decode(_progress, buffer.data());
        }
    }

    void _persist_progress()
    {
        bn::sram sram;
        bn::array<std::uint8_t, save_data::SIZE> buffer;
        (void)save_data::encode(_progress, buffer.data());
        sram.write(buffer);
    }

    void _trigger_shake()
    {
        _shake_timer = SHAKE_FRAMES;
    }

    void _update_shake()
    {
        if(_shake_timer > 0)
        {
            --_shake_timer;
            _camera.set_x((_shake_timer % 2 == 0) ? bn::fixed(2) : bn::fixed(-2));
            return;
        }

        _camera.set_x(0);
    }

    void _spawn_dust(bn::fixed_point origin)
    {
        for(int i = 0; i < 4; ++i)
        {
            if(_dust.full())
            {
                break;
            }

            DustPuff puff;
            puff.sprite = make_sprite(bn::sprite_items::ghost,
                                      origin.x() + bn::fixed(i * 4 - 6), origin.y(), 3);
            puff.vy = bn::fixed(-1) / 4;
            puff.life = DUST_LIFE;
            _dust.push_back(puff);
        }
    }

    void _update_dust()
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

    void _update_parallax()
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

    void _apply_parallax_to_backdrop()
    {
        const level::Def& def = _bridge.level_def();
        const bn::fixed back_shift = _parallax_shift.safe_multiplication(def.parallax_back);
        const bn::fixed front_shift = _parallax_shift;

        for(int i = 0; i < _sky.size(); ++i)
        {
            _sky[i].set_x(_sky_base_x[i] - back_shift / 4);
            _sky[i].set_y(_sky_base_y[i]);
        }

        for(int i = 0; i < _clouds.size(); ++i)
        {
            bn::sprite_ptr& cloud = _clouds[i];
            bn::fixed x = cloud.position().x() + bn::fixed(1) / 8;

            if(x > 130)
            {
                x = -130;
            }

            cloud.set_x(x);
        }

        for(int i = 0; i < _hills.size(); ++i)
        {
            _hills[i].set_x(_hill_base_x[i] - back_shift / 2);
            _hills[i].set_y(_hill_base_y[i]);
        }

        for(int i = 0; i < _ground.size(); ++i)
        {
            _ground[i].set_x(_ground_base_x[i] - front_shift / 3);
            _ground[i].set_y(_ground_base_y[i]);
        }

        for(int i = 0; i < _water.size(); ++i)
        {
            _water[i].set_x(_water_base_x[i] - front_shift / 2);
            _water[i].set_y(_water_base_y[i]);
        }
    }

    [[nodiscard]] engine::Id _active_engine() const
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

    void _sync_train_sprite()
    {
        if(! _train.visible())
        {
            return;
        }

        const engine::Id id = _active_engine();
        const int chuff = (_frame / 10) % engine::FRAMES_PER_ENGINE;
        _train.set_item(bn::sprite_items::train, engine::train_frame_base(id) + chuff);
    }

    void _set_weight_digits(int value, bool visible)
    {
        const int clamped = value < 0 ? 0 : (value > 99 ? 99 : value);
        _weight_digits[0].set_item(bn::sprite_items::digit, clamped / 10);
        _weight_digits[0].set_position(-4, 66);
        _weight_digits[0].set_visible(visible && clamped >= 10);
        _weight_digits[1].set_item(bn::sprite_items::digit, clamped % 10);
        _weight_digits[1].set_position(clamped >= 10 ? 4 : 0, 66);
        _weight_digits[1].set_visible(visible);
    }

    void _step_level(const int delta)
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
    }

    void _step_engine(const int delta)
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

    bool _dir_repeat(bool pressed, bool held, int& counter)
    {
        if(pressed)
        {
            counter = 0;
            return true;
        }

        if(held)
        {
            ++counter;

            if(counter > CURSOR_REPEAT_DELAY &&
               (counter - CURSOR_REPEAT_DELAY) % CURSOR_REPEAT_RATE == 0)
            {
                return true;
            }

            return false;
        }

        counter = 0;
        return false;
    }

    void _place_splash_tiles()
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

    void _set_splash_visible(bool visible)
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

    void _place_backdrop()
    {
        const int sky_x[] = { -96, -32, 32, 96 };
        const int sky_y[] = { -56, 8 };

        for(int y : sky_y)
        {
            for(int x : sky_x)
            {
                bn::sprite_ptr tile = bn::sprite_items::sky.create_sprite(x, y);
                tile.set_z_order(30);
                _sky.push_back(tile);
                _sky_base_x.push_back(x);
                _sky_base_y.push_back(y);
            }
        }

        _clouds.push_back(bn::sprite_items::cloud.create_sprite(-80, -64));
        _clouds.push_back(bn::sprite_items::cloud.create_sprite(24, -72));
        _clouds.back().set_horizontal_scale(bn::fixed(3) / 2);

        for(bn::sprite_ptr& cloud : _clouds)
        {
            cloud.set_z_order(25);
        }
    }

    void _animate_backdrop()
    {
        _update_parallax();

        for(int i = 0; i < _clouds.size(); ++i)
        {
            bn::sprite_ptr& cloud = _clouds[i];
            bn::fixed x = cloud.position().x() + bn::fixed(1) / 8;

            if(x > 130)
            {
                x = -130;
            }

            cloud.set_x(x);
        }

        if(_screen == Screen::playing || _screen == Screen::engine_select ||
           _screen == Screen::level_select || _screen == Screen::level_complete ||
           _screen == Screen::level_failed)
        {
            _apply_parallax_to_backdrop();
        }
    }

    void _show_title()
    {
        _screen = Screen::title;
        _play_mode = PlayMode::build;
        _hide_gameplay();
        _reset_scenery_layout();
        _set_backdrop_visible(false);
        _set_splash_visible(true);
        _logo.set_visible(true);
        _logo.set_position(0, -52);
        _banner.set_visible(false);
        _panel.set_visible(true);
        _panel.set_item(bn::sprite_items::panel, PANEL_DIALOG);
        _panel.set_position(0, 58);
        _label.set_visible(true);
        _label.set_position(0, 60);
        _label.set_item(bn::sprite_items::label, LABEL_START);
        _hint_label.set_visible(true);
        _hint_label.set_position(0, 42);
        _hint_label.set_item(bn::sprite_items::label, LABEL_HINT_NODE);
        _hint_secondary.set_visible(true);
        _hint_secondary.set_position(0, 50);
        _hint_secondary.set_item(bn::sprite_items::label, LABEL_HINT_TEST);
        _menu_row.set_visible(false);
        _menu_arrow.set_visible(false);
        _train.set_visible(false);
        _hatt.set_visible(false);
        play_music(MusicCue::island_line, _music_cue);
    }

    void _show_level_select()
    {
        _screen = Screen::level_select;
        _hide_gameplay();
        _reset_scenery_layout();
        _set_splash_visible(false);
        _set_backdrop_visible(true);
        _banner.set_visible(true);
        _banner.set_item(bn::sprite_items::banner, BANNER_LEVEL_SELECT);
        _banner.set_position(0, -52);
        _panel.set_visible(true);
        _panel.set_item(bn::sprite_items::panel, PANEL_SELECT);
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
        _hatt.set_item(bn::sprite_items::hatt, HATT_STERN);
        _portrait.set_visible(false);
        _set_weight_digits(0, false);
        _node_count_digits[0].set_visible(false);
        _node_count_digits[1].set_visible(false);
        _strut_count_digits[0].set_visible(false);
        _strut_count_digits[1].set_visible(false);
    }

    void _show_engine_select()
    {
        _screen = Screen::engine_select;
        _hide_gameplay();
        _set_splash_visible(false);
        _set_backdrop_visible(true);
        _banner.set_visible(true);
        _banner.set_item(bn::sprite_items::banner, BANNER_ENGINE_SELECT);
        _banner.set_position(0, -48);
        _panel.set_visible(true);
        _panel.set_item(bn::sprite_items::panel, PANEL_SELECT);
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
        _hatt.set_item(bn::sprite_items::hatt, HATT_STERN);
        _node_count_digits[0].set_visible(false);
        _node_count_digits[1].set_visible(false);
        _strut_count_digits[0].set_visible(false);
        _strut_count_digits[1].set_visible(false);
        play_music(MusicCue::yard, _music_cue);
    }

    void _start_level(int level_index)
    {
        _selected_level = level_index;
        _bridge.load_level(level_index);
        _bridge.set_engine(_selected_engine);
        _screen = Screen::playing;
        _play_mode = PlayMode::build;
        _selected.reset();
        _pause_option = PauseOption::resume;
        _build_hint_timer = 0;
        _cursor_pos = _bridge.nodes()[bridge::START_NODE].position;
        _cursor_pos.set_x(_cursor_pos.x() + 16);
        _layout_scenery_for_level();
        _show_gameplay();
        _sync_node_sprites();
        _sync_strut_sprites();
        play_music(MusicCue::workshop, _music_cue);
    }

    void _show_gameplay()
    {
        _set_splash_visible(false);
        _banner.set_visible(false);
        _menu_row.set_visible(false);
        _menu_arrow.set_visible(false);
        _hatt.set_visible(false);
        _panel.set_visible(true);
        _panel.set_item(bn::sprite_items::panel, PANEL_HUD);
        _panel.set_position(0, 66);
        _label.set_visible(true);
        _label.set_position(0, 68);
        _label.set_item(bn::sprite_items::label, LABEL_BUILD);
        _hint_secondary.set_visible(false);
        _update_build_hints();
        _cursor.set_visible(true);
        _train.set_visible(false);
        _set_backdrop_visible(true);
        _set_scenery_visible(true);
    }

    void _hide_gameplay()
    {
        _cursor.set_visible(false);
        _train.set_visible(false);
        _set_scenery_visible(false);

        for(bn::sprite_ptr& sprite : _node_sprites)
        {
            sprite.set_visible(false);
        }

        for(bn::sprite_ptr& sprite : _strut_sprites)
        {
            sprite.set_visible(false);
        }
    }

    void _set_backdrop_visible(bool visible)
    {
        for(bn::sprite_ptr& tile : _sky)
        {
            tile.set_visible(visible);
        }

        for(bn::sprite_ptr& cloud : _clouds)
        {
            cloud.set_visible(visible);
        }
    }

    void _set_scenery_visible(bool visible)
    {
        for(bn::sprite_ptr& tile : _ground)
        {
            tile.set_visible(visible);
        }

        for(bn::sprite_ptr& tile : _hills)
        {
            tile.set_visible(visible);
        }

        for(bn::sprite_ptr& tile : _water)
        {
            tile.set_visible(visible);
        }
    }

    void _enter_pause()
    {
        _screen = Screen::pause;
        _set_splash_visible(false);
        _pause_option = PauseOption::resume;
        _banner.set_visible(true);
        _banner.set_item(bn::sprite_items::banner, BANNER_LEVEL_SELECT);
        _banner.set_position(0, -48);
        _panel.set_visible(true);
        _panel.set_item(bn::sprite_items::panel, PANEL_SELECT);
        _panel.set_position(0, 12);
        _label.set_visible(false);
        _hint_label.set_visible(false);
        _hint_secondary.set_visible(false);
        _menu_row.set_visible(true);
        _menu_row.set_item(bn::sprite_items::menu, MENU_PAUSE_BASE + int(_pause_option));
        _menu_row.set_position(0, 0);
        _menu_arrow.set_visible(true);
        _menu_arrow.set_position(-38, 8);
        _cursor.set_visible(false);
        _hatt.set_visible(true);
        _hatt.set_position(56, 8);
        _hatt.set_item(bn::sprite_items::hatt, HATT_FURIOUS);

        if(bn::music::playing())
        {
            bn::music::set_volume(MUSIC_PAUSE_VOLUME);
        }
    }

    void _leave_pause()
    {
        _screen = Screen::playing;
        _banner.set_visible(false);
        _menu_row.set_visible(false);
        _menu_arrow.set_visible(false);
        _hatt.set_visible(false);
        _panel.set_visible(true);
        _panel.set_item(bn::sprite_items::panel, PANEL_HUD);
        _panel.set_position(0, 66);
        _label.set_visible(true);
        _label.set_position(0, 68);
        _label.set_item(bn::sprite_items::label, LABEL_BUILD);
        _cursor.set_visible(true);

        if(bn::music::playing())
        {
            bn::music::set_volume(MUSIC_VOLUME);
        }
    }

    void _show_level_complete()
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

        _persist_progress();

        _banner.set_visible(true);
        _banner.set_item(bn::sprite_items::banner, _pick_win_banner());
        _banner.set_position(0, -28);
        _panel.set_visible(true);
        _panel.set_item(bn::sprite_items::panel, PANEL_DIALOG);
        _panel.set_position(0, 48);
        _label.set_visible(true);
        _label.set_position(0, 50);
        _label.set_item(bn::sprite_items::label,
                        _show_unlock_flash ? LABEL_NEW : _pick_win_label());
        _hint_label.set_visible(false);
        _hint_secondary.set_visible(false);
        _menu_arrow.set_visible(false);
        _cursor.set_visible(false);
        _train.set_visible(true);
        set_train_sprite_position(_train, _bridge.train_position());
        play_music(MusicCue::none, _music_cue);
        bn::sound_items::useful.play();
        bn::sound_items::whistle.play();
        _hatt.set_visible(true);
        _hatt.set_position(-56, 8);
        _hatt.set_item(bn::sprite_items::hatt, HATT_STERN);
        _sync_train_sprite();
        _set_weight_digits(0, false);
    }

    void _show_level_failed()
    {
        _screen = Screen::level_failed;
        _set_splash_visible(false);
        _banner.set_visible(true);
        _banner.set_item(bn::sprite_items::banner, _pick_fail_banner());
        _banner.set_position(0, -28);
        _panel.set_visible(true);
        _panel.set_item(bn::sprite_items::panel, PANEL_DIALOG);
        _panel.set_position(0, 48);
        _label.set_visible(true);
        _label.set_position(0, 50);
        _label.set_item(bn::sprite_items::label, _pick_fail_label());
        _hint_label.set_visible(false);
        _hint_secondary.set_visible(false);
        _cursor.set_visible(false);
        _train.set_visible(_train.position().y() < bridge::FALL_Y_LIMIT);
        _sync_train_sprite();
        play_music(MusicCue::none, _music_cue);
        bn::sound_items::snap.play();
        bn::sound_items::delay.play();
        bn::sound_items::splash.play();
        _hatt.set_visible(true);
        _hatt.set_position(-56, 8);
        _hatt.set_item(bn::sprite_items::hatt, HATT_FURIOUS);
        _set_weight_digits(0, false);
    }

    void _show_all_complete()
    {
        _screen = Screen::all_complete;
        _set_splash_visible(false);
        _banner.set_visible(true);
        _banner.set_item(bn::sprite_items::banner, BANNER_HATT_OK);
        _banner.set_position(0, -36);
        _panel.set_visible(true);
        _panel.set_item(bn::sprite_items::panel, PANEL_DIALOG);
        _panel.set_position(0, 40);
        _label.set_visible(true);
        _label.set_position(0, 42);
        _label.set_item(bn::sprite_items::label, LABEL_USEFUL);
        _hint_label.set_visible(false);
        _hint_secondary.set_visible(false);
        _menu_row.set_visible(false);
        _menu_arrow.set_visible(false);
        _train.set_visible(true);
        _train.set_position(0, game::TRACK_Y + game::TRAIN_Y_OFFSET);
        _hatt.set_visible(true);
        _hatt.set_position(56, 16);
        _hatt.set_item(bn::sprite_items::hatt, HATT_STERN);
        play_music(MusicCue::island_line, _music_cue);
        bn::sound_items::useful.play();
    }

    void _update_title()
    {
        _label.set_visible((_frame / 18) % 2 == 0);

        if(bn::keypad::start_pressed() || bn::keypad::a_pressed())
        {
            _show_level_select();
        }
    }

    void _update_level_select()
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

    void _update_engine_select()
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
                _start_level(_pending_level);
            }

            return;
        }

        if(bn::keypad::b_pressed())
        {
            _show_level_select();
        }
    }

    void _update_pause()
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

        _menu_row.set_item(bn::sprite_items::menu, MENU_PAUSE_BASE + int(_pause_option));

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
                    _sync_node_sprites();
                    _sync_strut_sprites();
                    play_music(MusicCue::workshop, _music_cue);
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

    void _update_level_complete()
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

    void _update_level_failed()
    {
        if(bn::keypad::a_pressed() || bn::keypad::start_pressed() || bn::keypad::b_pressed())
        {
            _leave_test();
        }
    }

    void _update_all_complete()
    {
        if(bn::keypad::a_pressed() || bn::keypad::start_pressed() || bn::keypad::b_pressed())
        {
            _show_title();
        }
    }

    void _update_playing()
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

        _sync_node_sprites();
        _sync_strut_sprites();
        _update_preview();
        _update_hud();
        _update_toast();
        _animate_water();
    }

    void _set_counter(bn::vector<bn::sprite_ptr, 2>& digits, int value, bn::fixed x_base)
    {
        const int clamped = value < 0 ? 0 : (value > 99 ? 99 : value);
        digits[0].set_item(bn::sprite_items::digit, clamped / 10);
        digits[0].set_position(x_base, 66);
        digits[0].set_visible(_screen == Screen::playing);
        digits[1].set_item(bn::sprite_items::digit, clamped % 10);
        digits[1].set_position(x_base + 8, 66);
        digits[1].set_visible(_screen == Screen::playing);
    }

    void _show_toast(ToastKind kind)
    {
        _toast_kind = kind;
        _toast_timer = TOAST_FRAMES;
        _toast.set_visible(_screen == Screen::playing);

        switch(kind)
        {
            case ToastKind::dup:
                _toast.set_item(bn::sprite_items::toast, TOAST_DUP);
                break;

            case ToastKind::weak:
                _toast.set_item(bn::sprite_items::toast, TOAST_WEAK);
                break;

            case ToastKind::deleted:
                _toast.set_item(bn::sprite_items::toast, TOAST_DEL);
                break;

            default:
                break;
        }
    }

    void _update_toast()
    {
        if(_toast_timer <= 0 || _screen != Screen::playing)
        {
            _toast.set_visible(false);
            _toast_kind = ToastKind::none;
            return;
        }

        --_toast_timer;
        _toast.set_visible(true);
    }

    void _update_preview()
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
                _bridge.preview_strut(_selected.value(), hovered.value());

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
            const int frame = max_tension < load ? STRUT_PREVIEW_WEAK : STRUT_PREVIEW_OK;
            position_strut_sprite(_preview_strut, from, to, frame);
            return;
        }

        _preview_strut.set_visible(false);
    }

    void _place_scenery()
    {
        const int ground_x[] = { -120, -88, -56, -24, 8, 40, 72, 104 };
        const int water_x[] = { -56, -24, 8, 40, 72, 104, -88, 120 };

        for(int x : ground_x)
        {
            bn::sprite_ptr tile = bn::sprite_items::ground.create_sprite(x, game::GROUND_TILE_Y);
            tile.set_z_order(20);
            _ground.push_back(tile);
            _ground_base_x.push_back(x);
            _ground_base_y.push_back(game::GROUND_TILE_Y);
        }

        for(int x : { -96, -32, 32, 96 })
        {
            bn::sprite_ptr tile = bn::sprite_items::scenery_far.create_sprite(x, 24);
            tile.set_z_order(22);
            _hills.push_back(tile);
            _hill_base_x.push_back(x);
            _hill_base_y.push_back(24);
        }

        for(int x : water_x)
        {
            bn::sprite_ptr tile = bn::sprite_items::water.create_sprite(x, 62);
            tile.set_z_order(21);
            _water.push_back(tile);
            _water_base_x.push_back(x);
            _water_base_y.push_back(62);
        }
    }

    void _reset_scenery_layout()
    {
        for(int i = 0; i < _ground.size(); ++i)
        {
            _ground_base_y[i] = game::GROUND_TILE_Y;
            _ground[i].set_y(game::GROUND_TILE_Y);
        }

        for(int i = 0; i < _water.size(); ++i)
        {
            _water_base_y[i] = 62;
            _water[i].set_y(62);
        }

        _apply_parallax_to_backdrop();
    }

    void _layout_scenery_for_level()
    {
        const level::Def& def = _bridge.level_def();
        const bn::fixed bank_ground_y = def.deck_y + (game::GROUND_TILE_Y - game::TRACK_Y);
        const bn::fixed water_y = bank_ground_y + 6;

        for(int i = 0; i < _ground.size(); ++i)
        {
            const bn::fixed x = _ground_base_x[i];
            const bool bank_tile = x < -48 || x > 48;
            const bn::fixed y = bank_tile ? bank_ground_y : game::GROUND_TILE_Y;
            _ground_base_y[i] = y;
            _ground[i].set_y(y);
        }

        for(int i = 0; i < _water.size(); ++i)
        {
            _water_base_y[i] = water_y;
            _water[i].set_y(water_y);
        }

        _apply_parallax_to_backdrop();
    }

    void _animate_water()
    {
        const int frame = (_frame / 16) % 2;

        for(bn::sprite_ptr& tile : _water)
        {
            tile.set_item(bn::sprite_items::water, frame);
        }
    }

    void _update_build_hints()
    {
        if(_screen != Screen::playing || _play_mode != PlayMode::build)
        {
            _hint_label.set_visible(false);
            return;
        }

        if(!_build_hints_seen)
        {
            ++_build_hint_timer;

            if(_build_hint_timer >= BUILD_HINT_FRAMES ||
               bn::keypad::any_pressed())
            {
                _build_hints_seen = true;
            }

            const int phase = (_build_hint_timer / 60) % 3;
            int frame = LABEL_HINT_NODE;

            if(phase == 1)
            {
                frame = LABEL_HINT_TEST;
            }
            else if(phase == 2)
            {
                frame = LABEL_HINT_PAUSE;
            }

            _hint_label.set_visible(true);
            _hint_label.set_position(0, 52);
            _hint_label.set_item(bn::sprite_items::label, frame);
            return;
        }

        _hint_label.set_visible(true);
        _hint_label.set_position(0, 52);
        _hint_label.set_item(bn::sprite_items::label, LABEL_HINT_PAUSE);
    }

    void _update_build()
    {
        bn::fixed speed = bn::keypad::r_held() ? CURSOR_FAST : CURSOR_SPEED;
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

        _cursor_pos.set_x(clamp(_cursor_pos.x(), -112, 112));
        _cursor_pos.set_y(clamp(_cursor_pos.y(), -72, game::UNDERGROUND_Y));

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

                    _show_toast(ToastKind::deleted);
                }
            }
            else
            {
                _selected.reset();
            }
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

    void _handle_confirm(bn::optional<int> hovered, bn::fixed_point place_at)
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
                _show_toast(ToastKind::full);
                return;
            }

            const bn::fixed_point snapped = game::snap_point(place_at);
            const bn::optional<int> created =
                _bridge.add_node(snapped.x(), snapped.y(), false);

            if(!created)
            {
                _show_toast(ToastKind::full);
                return;
            }

            node_id = created.value();
        }

        if(_selected && _selected.value() != node_id)
        {
            const bridge::StrutPreview preview = _bridge.preview_strut(_selected.value(), node_id);

            switch(preview.status)
            {
                case bridge::StrutPreviewStatus::duplicate:
                    _show_toast(ToastKind::dup);
                    break;

                case bridge::StrutPreviewStatus::struts_full:
                    _show_toast(ToastKind::full);
                    break;

                case bridge::StrutPreviewStatus::ok:
                    if(preview.max_tension < _max_train_load())
                    {
                        _show_toast(ToastKind::weak);
                    }

                    (void)_bridge.add_strut(_selected.value(), node_id);
                    break;

                default:
                    break;
            }
        }

        _selected = node_id;
    }

    void _enter_test()
    {
        _play_mode = PlayMode::test;
        _selected.reset();
        _cursor.set_visible(false);
        _ghost.set_visible(false);
        _ring.set_visible(false);
        _preview_strut.set_visible(false);
        _train.set_visible(true);
        _train_prev_x = _bridge.train_position().x();
        (void)_bridge.begin_test();
        set_train_sprite_position(_train, _bridge.train_position());
        _chuff_cooldown = 0;

        if(_bridge.status() == bridge::SimStatus::no_path)
        {
            play_music(MusicCue::none, _music_cue);
            bn::sound_items::delay.play();
            _no_go_timer = 60;
            _leave_test();
            return;
        }
        else
        {
            play_music(MusicCue::full_steam, _music_cue);
            bn::sound_items::peep.play();
        }
    }

    void _leave_test()
    {
        _bridge.end_test();
        _play_mode = PlayMode::build;
        _train.set_visible(false);
        _cursor.set_visible(true);
        _cursor.set_position(_cursor_pos);
        play_music(MusicCue::workshop, _music_cue);
    }

    void _update_test()
    {
        const bn::fixed prev_x = _train.position().x();
        const bridge::SimStatus prev_status = _bridge.status();
        (void)_bridge.update();
        set_train_sprite_position(_train, _bridge.train_position());

        const bridge::SimStatus status = _bridge.status();

        if(status == bridge::SimStatus::failed_fall && prev_status == bridge::SimStatus::running)
        {
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
            _show_level_failed();
            return;
        }

        if(bn::keypad::start_pressed() || bn::keypad::b_pressed())
        {
            _leave_test();
        }
    }

    void _sync_node_sprites()
    {
        const auto& nodes = _bridge.nodes();

        while(_node_sprites.size() > nodes.size())
        {
            _node_sprites.pop_back();
        }

        while(_node_sprites.size() < nodes.size())
        {
            _node_sprites.push_back(make_sprite(bn::sprite_items::node, 0, 0, Z_NODE));
        }

        for(int i = 0; i < nodes.size(); ++i)
        {
            bn::sprite_ptr& sprite = _node_sprites[i];
            sprite.set_position(nodes[i].position);

            if(nodes[i].is_anchored)
            {
                sprite.set_item(bn::sprite_items::anchor);
            }
            else
            {
                sprite.set_item(bn::sprite_items::node);
            }

            sprite.set_visible(_screen == Screen::playing);
        }

        if(_screen == Screen::playing && _play_mode == PlayMode::build && _selected)
        {
            _ring.set_position(nodes[_selected.value()].position);
            _ring.set_visible(true);
            const bn::fixed pulse = (_frame % 24 < 12) ? bn::fixed(1) : bn::fixed(9) / 8;
            _ring.set_scale(pulse);
        }
        else
        {
            _ring.set_visible(false);
        }
    }

    void _sync_strut_sprites()
    {
        const auto& nodes = _bridge.nodes();
        const auto& struts = _bridge.struts();

        while(_strut_sprites.size() > struts.size())
        {
            _strut_sprites.pop_back();
        }

        while(_strut_sprites.size() < struts.size())
        {
            _strut_sprites.push_back(make_sprite(bn::sprite_items::strut, 0, 0, Z_STRUT));
        }

        for(int i = 0; i < struts.size(); ++i)
        {
            const bridge::Strut& strut = struts[i];
            const bn::fixed_point a = nodes[strut.node_a].position;
            const bn::fixed_point b = nodes[strut.node_b].position;
            const bool hot = _play_mode == PlayMode::test &&
                strut.current_tension * 2 > strut.max_tension;
            bn::sprite_ptr& sprite = _strut_sprites[i];
            position_strut_sprite(sprite, a, b, hot ? STRUT_HOT : STRUT_IDLE);

            if(hot)
            {
                const bn::fixed pulse = (_frame % 16 < 8) ? bn::fixed(1) : bn::fixed(11) / 10;
                sprite.set_horizontal_scale(sprite.horizontal_scale() * pulse);
            }

            sprite.set_visible(_screen == Screen::playing);
        }
    }

    void _update_hud()
    {
        int frame = LABEL_BUILD;

        if(_no_go_timer > 0)
        {
            --_no_go_timer;
            frame = LABEL_NO_GO;
        }
        else if(_play_mode == PlayMode::test)
        {
            switch(_bridge.status())
            {
                case bridge::SimStatus::success:
                    frame = LABEL_USEFUL;
                    break;

                case bridge::SimStatus::failed_fall:
                    frame = LABEL_ASHES;
                    break;

                case bridge::SimStatus::no_path:
                    frame = LABEL_NO_GO;
                    break;

                default:
                    frame = LABEL_TEST;
                    break;
            }
        }

        _label.set_item(bn::sprite_items::label, frame);
        _set_counter(_node_count_digits, _bridge.player_node_count(), -20);
        _set_counter(_strut_count_digits, _bridge.struts().size(), 12);
        _set_weight_digits(_max_train_load().integer(), true);
    }

    void _animate_train()
    {
        const int chuff_frame = (_frame / CHUFF_FRAMES) % 2;
        _sync_train_sprite();

        if(_play_mode == PlayMode::test &&
           _bridge.status() == bridge::SimStatus::running &&
           chuff_frame == 0 &&
           _last_chuff_frame != 0 &&
           _chuff_cooldown <= 0)
        {
            bn::sound_items::chuff.play();
            _chuff_cooldown = CHUFF_FRAMES / 2;
        }

        _last_chuff_frame = chuff_frame;

        if(_chuff_cooldown > 0)
        {
            --_chuff_cooldown;
        }
    }

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
    bn::fixed_point _cursor_pos;
    bn::optional<int> _selected;
    int _selected_level = 0;
    PauseOption _pause_option = PauseOption::resume;
    MusicCue _music_cue = MusicCue::none;
    int _frame = 0;
    int _menu_up_counter = 0;
    int _menu_down_counter = 0;
    bool _train_facing_right = true;
    bn::fixed _train_prev_x = 0;
    int _chuff_cooldown = 0;
    int _last_chuff_frame = 1;
    int _toast_timer = 0;
    ToastKind _toast_kind = ToastKind::none;
    bool _build_hints_seen = false;
    int _build_hint_timer = 0;
    int _no_go_timer = 0;

    bn::sprite_ptr _cursor;
    bn::sprite_ptr _train;
    bn::sprite_ptr _label;
    bn::sprite_ptr _hint_label;
    bn::sprite_ptr _hint_secondary;
    bn::sprite_ptr _panel;
    bn::sprite_ptr _banner;
    bn::sprite_ptr _logo;
    bn::sprite_ptr _menu_row;
    bn::sprite_ptr _menu_arrow;
    bn::sprite_ptr _hatt;
    bn::sprite_ptr _ghost;
    bn::sprite_ptr _ring;
    bn::sprite_ptr _preview_strut;
    bn::sprite_ptr _toast;
    bn::sprite_ptr _portrait;
    bn::camera_ptr _camera;
    bn::vector<DustPuff, MAX_DUST> _dust;
    bn::vector<bn::sprite_ptr, 2> _node_count_digits;
    bn::vector<bn::sprite_ptr, 2> _strut_count_digits;
    bn::vector<bn::sprite_ptr, 2> _weight_digits;
    bn::vector<bn::sprite_ptr, bridge::MAX_NODES> _node_sprites;
    bn::vector<bn::sprite_ptr, bridge::MAX_STRUTS> _strut_sprites;
    bn::vector<bn::sprite_ptr, 8> _ground;
    bn::vector<bn::sprite_ptr, 4> _hills;
    bn::vector<bn::sprite_ptr, 8> _water;
    bn::vector<bn::sprite_ptr, 4> _splash;
    bn::vector<bn::sprite_ptr, 8> _sky;
    bn::vector<bn::sprite_ptr, 2> _clouds;
    bn::vector<bn::fixed, 8> _sky_base_x;
    bn::vector<bn::fixed, 8> _sky_base_y;
    bn::vector<bn::fixed, 4> _hill_base_x;
    bn::vector<bn::fixed, 4> _hill_base_y;
    bn::vector<bn::fixed, 8> _ground_base_x;
    bn::vector<bn::fixed, 8> _ground_base_y;
    bn::vector<bn::fixed, 8> _water_base_x;
    bn::vector<bn::fixed, 8> _water_base_y;
};

} // namespace

int main()
{
    bn::core::init();

    Game game;

    while(true)
    {
        game.update();
        bn::core::update();
    }
}
