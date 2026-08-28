#include "bn_core.h"
#include "bn_color.h"
#include "bn_bg_palettes.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_optional.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "bn_sprite_items_anchor.h"
#include "bn_sprite_items_banner.h"
#include "bn_sprite_items_cursor.h"
#include "bn_sprite_items_ground.h"
#include "bn_sprite_items_label.h"
#include "bn_sprite_items_menu.h"
#include "bn_sprite_items_node.h"
#include "bn_sprite_items_strut.h"
#include "bn_sprite_items_train.h"
#include "bn_sprite_items_water.h"

#include "bridge_system.h"

namespace
{

constexpr bn::fixed CURSOR_SPEED = 2;
constexpr bn::fixed CURSOR_FAST = 4;
constexpr bn::fixed STRUT_SPRITE_WIDTH = 16;

constexpr int LABEL_BUILD = 0;
constexpr int LABEL_TEST = 1;
constexpr int LABEL_FAIL = 2;
constexpr int LABEL_WIN = 3;
constexpr int LABEL_NO_GO = 4;
constexpr int LABEL_PEEP = 5;
constexpr int LABEL_ASHES = 6;
constexpr int LABEL_MENU = 7;
constexpr int LABEL_LV1 = 8;
constexpr int LABEL_LV2 = 9;
constexpr int LABEL_LV3 = 10;

constexpr int BANNER_TITLE = 0;
constexpr int BANNER_LEVEL_SELECT = 1;
constexpr int BANNER_VICTORY = 2;
constexpr int BANNER_AI = 3;
constexpr int MENU_LEVEL_COUNT = 3;
constexpr int MENU_PAUSE_BASE = 3;

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

class Game
{
public:
    Game() :
        _cursor(bn::sprite_items::cursor.create_sprite(0, 0)),
        _train(bn::sprite_items::train.create_sprite(_bridge.train_position())),
        _label(bn::sprite_items::label.create_sprite(-104, -72)),
        _banner(bn::sprite_items::banner.create_sprite(0, -48)),
        _banner_ai(bn::sprite_items::banner.create_sprite(0, 56)),
        _menu_row(bn::sprite_items::menu.create_sprite(0, 8)),
        _menu_arrow(bn::sprite_items::cursor.create_sprite(-56, 8))
    {
        bn::bg_palettes::set_transparent_color(bn::color(8, 14, 20));

        _cursor.set_z_order(-32767);
        _train.set_z_order(-10);
        _train.set_visible(false);
        _label.set_z_order(-20);
        _banner.set_z_order(-30);
        _banner_ai.set_z_order(-29);
        _menu_row.set_z_order(-25);
        _menu_arrow.set_z_order(-24);

        _place_scenery();
        _show_title();
        _banner_ai.set_item(bn::sprite_items::banner, BANNER_AI);
        _banner_ai.set_position(0, 56);
    }

    void update()
    {
        ++_frame;

        switch(_screen)
        {
            case Screen::title:
                _update_title();
                break;

            case Screen::level_select:
                _update_level_select();
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
        playing,
        pause,
        level_complete,
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

    void _show_title()
    {
        _screen = Screen::title;
        _play_mode = PlayMode::build;
        _hide_gameplay();
        _banner.set_visible(true);
        _banner.set_item(bn::sprite_items::banner, BANNER_TITLE);
        _banner.set_position(0, -40);
        _menu_row.set_visible(false);
        _menu_arrow.set_visible(false);
        _label.set_visible(true);
        _label.set_position(0, 48);
        _label.set_item(bn::sprite_items::label, LABEL_MENU);
        _train.set_visible(true);
        _train.set_position(-40, 16);
        _banner_ai.set_visible(true);
    }

    void _show_level_select()
    {
        _screen = Screen::level_select;
        _hide_gameplay();
        _banner_ai.set_visible(false);
        _banner.set_visible(true);
        _banner.set_item(bn::sprite_items::banner, BANNER_LEVEL_SELECT);
        _banner.set_position(0, -52);
        _menu_row.set_visible(true);
        _menu_row.set_item(bn::sprite_items::menu, _selected_level);
        _menu_row.set_position(0, 12);
        _menu_arrow.set_visible(true);
        _menu_arrow.set_position(-20, 12);
        _label.set_visible(true);
        _label.set_position(0, -8);
        _label.set_item(bn::sprite_items::label, LABEL_LV1 + _selected_level);
        _train.set_visible(true);
        _train.set_position(48, 40);
    }

    void _start_level(int level_index)
    {
        _selected_level = level_index;
        _bridge.load_level(level_index);
        _screen = Screen::playing;
        _play_mode = PlayMode::build;
        _selected.reset();
        _pause_option = PauseOption::resume;
        _show_gameplay();
        _sync_node_sprites();
        _sync_strut_sprites();
        _label.set_item(bn::sprite_items::label, LABEL_BUILD);
    }

    void _show_gameplay()
    {
        _banner.set_visible(false);
        _banner_ai.set_visible(false);
        _menu_row.set_visible(false);
        _menu_arrow.set_visible(false);
        _label.set_visible(true);
        _label.set_position(-104, -72);
        _cursor.set_visible(true);
        _train.set_visible(false);
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

    void _set_scenery_visible(bool visible)
    {
        for(bn::sprite_ptr& tile : _ground)
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
        _pause_option = PauseOption::resume;
        _banner.set_visible(true);
        _banner.set_item(bn::sprite_items::banner, BANNER_LEVEL_SELECT);
        _banner.set_position(0, -52);
        _label.set_visible(false);
        _menu_row.set_visible(true);
        _menu_row.set_item(bn::sprite_items::menu, MENU_PAUSE_BASE + int(_pause_option));
        _menu_row.set_position(0, 8);
        _menu_arrow.set_visible(false);
        _cursor.set_visible(false);
    }

    void _leave_pause()
    {
        _screen = Screen::playing;
        _banner.set_visible(false);
        _menu_row.set_visible(false);
        _menu_arrow.set_visible(false);
        _label.set_visible(true);
        _label.set_position(-104, -72);
        _label.set_item(bn::sprite_items::label, LABEL_BUILD);
        _cursor.set_visible(true);
    }

    void _show_level_complete()
    {
        _screen = Screen::level_complete;
        _banner.set_visible(true);
        _banner.set_item(bn::sprite_items::banner, BANNER_VICTORY);
        _banner.set_position(0, -32);
        _label.set_visible(true);
        _label.set_position(0, 40);
        _label.set_item(bn::sprite_items::label, LABEL_PEEP);
        _menu_arrow.set_visible(false);
        _cursor.set_visible(false);
        _train.set_visible(true);
        _train.set_position(_bridge.train_position());
    }

    void _show_all_complete()
    {
        _screen = Screen::all_complete;
        _banner.set_visible(true);
        _banner.set_item(bn::sprite_items::banner, BANNER_VICTORY);
        _banner.set_position(0, -40);
        _label.set_visible(true);
        _label.set_position(0, 16);
        _label.set_item(bn::sprite_items::label, LABEL_WIN);
        _menu_row.set_visible(false);
        _menu_arrow.set_visible(false);
        _train.set_visible(true);
        _train.set_position(0, 48);
    }

    void _update_title()
    {
        const bool blink = (_frame / 20) % 2 == 0;
        _label.set_visible(blink);

        if(bn::keypad::start_pressed())
        {
            _show_level_select();
        }
    }

    void _update_level_select()
    {
        if(bn::keypad::up_pressed())
        {
            _selected_level = (_selected_level + bridge::LEVEL_COUNT - 1) % bridge::LEVEL_COUNT;
            _menu_row.set_item(bn::sprite_items::menu, _selected_level);
            _label.set_item(bn::sprite_items::label, LABEL_LV1 + _selected_level);
        }

        if(bn::keypad::down_pressed())
        {
            _selected_level = (_selected_level + 1) % bridge::LEVEL_COUNT;
            _menu_row.set_item(bn::sprite_items::menu, _selected_level);
            _label.set_item(bn::sprite_items::label, LABEL_LV1 + _selected_level);
        }

        if(bn::keypad::a_pressed() || bn::keypad::start_pressed())
        {
            _start_level(_selected_level);
        }

        if(bn::keypad::b_pressed())
        {
            _show_title();
        }
    }

    void _update_pause()
    {
        if(bn::keypad::up_pressed())
        {
            _pause_option = PauseOption((int(_pause_option) + int(PauseOption::count) - 1) %
                                        int(PauseOption::count));
        }

        if(bn::keypad::down_pressed())
        {
            _pause_option = PauseOption((int(_pause_option) + 1) % int(PauseOption::count));
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
                    _leave_pause();
                    _sync_node_sprites();
                    _sync_strut_sprites();
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
            if(_bridge.current_level() + 1 >= bridge::LEVEL_COUNT)
            {
                _show_all_complete();
            }
            else
            {
                _start_level(_bridge.current_level() + 1);
            }
        }

        if(bn::keypad::b_pressed())
        {
            _show_level_select();
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
        }
        else
        {
            _update_test();
        }

        _sync_node_sprites();
        _sync_strut_sprites();
        _update_hud();
    }

    void _place_scenery()
    {
        const int ground_x[] = { -104, -72, 72, 104 };
        const int water_x[] = { -40, -8, 24, 56 };

        for(int x : ground_x)
        {
            bn::sprite_ptr tile = bn::sprite_items::ground.create_sprite(x, 56);
            tile.set_z_order(20);
            _ground.push_back(tile);
        }

        for(int x : water_x)
        {
            bn::sprite_ptr tile = bn::sprite_items::water.create_sprite(x, 74);
            tile.set_z_order(21);
            _water.push_back(tile);
        }
    }

    void _update_build()
    {
        bn::fixed speed = bn::keypad::r_held() ? CURSOR_FAST : CURSOR_SPEED;

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

        _cursor_pos.set_x(clamp(_cursor_pos.x(), -116, 116));
        _cursor_pos.set_y(clamp(_cursor_pos.y(), -76, 64));

        const bn::optional<int> hovered = _bridge.find_node_near(_cursor_pos);
        const bn::fixed_point draw_at = hovered ? _bridge.nodes()[hovered.value()].position : _cursor_pos;
        _cursor.set_position(draw_at);
        _cursor.set_visible(true);

        if(bn::keypad::a_pressed())
        {
            _handle_confirm(hovered);
        }

        if(bn::keypad::b_pressed())
        {
            _selected.reset();
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

    void _handle_confirm(bn::optional<int> hovered)
    {
        int node_id;

        if(hovered)
        {
            node_id = hovered.value();
        }
        else
        {
            const bn::optional<int> created = _bridge.add_node(_cursor_pos.x(), _cursor_pos.y(), false);

            if(!created)
            {
                return;
            }

            node_id = created.value();
        }

        if(_selected && _selected.value() != node_id)
        {
            (void)_bridge.add_strut(_selected.value(), node_id);
        }

        _selected = node_id;
    }

    void _enter_test()
    {
        _play_mode = PlayMode::test;
        _selected.reset();
        _cursor.set_visible(false);
        _train.set_visible(true);
        (void)_bridge.begin_test();
        _train.set_position(_bridge.train_position());
    }

    void _leave_test()
    {
        _bridge.end_test();
        _play_mode = PlayMode::build;
        _train.set_visible(false);
        _cursor.set_visible(true);
        _cursor.set_position(_cursor_pos);
    }

    void _update_test()
    {
        (void)_bridge.update();
        _train.set_position(_bridge.train_position());
        _train.set_visible(_train.position().y() < bridge::FALL_Y_LIMIT);

        const bridge::SimStatus status = _bridge.status();

        if(status == bridge::SimStatus::success)
        {
            _show_level_complete();
            return;
        }

        if(bn::keypad::start_pressed() || bn::keypad::b_pressed())
        {
            _leave_test();

            if(bn::keypad::select_pressed())
            {
                _bridge.reset_level();
            }
        }

        if(bn::keypad::select_pressed())
        {
            _leave_test();
            _bridge.reset_level();
            _enter_pause();
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
            _node_sprites.push_back(bn::sprite_items::node.create_sprite(0, 0));
        }

        for(int i = 0; i < nodes.size(); ++i)
        {
            bn::sprite_ptr& sprite = _node_sprites[i];
            sprite.set_position(nodes[i].position);
            sprite.set_z_order(5);

            if(nodes[i].is_anchored)
            {
                sprite.set_item(bn::sprite_items::anchor);
            }
            else
            {
                sprite.set_item(bn::sprite_items::node);
            }

            const bool selected = _selected && _selected.value() == i;
            const bool blink_off = selected && ((_frame / 8) % 2 == 0);
            sprite.set_visible(_screen == Screen::playing && !blink_off);
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
            bn::sprite_ptr sprite = bn::sprite_items::strut.create_sprite(0, 0);
            sprite.set_z_order(10);
            _strut_sprites.push_back(sprite);
        }

        for(int i = 0; i < struts.size(); ++i)
        {
            const bridge::Strut& strut = struts[i];
            const bn::fixed_point a = nodes[strut.node_a].position;
            const bn::fixed_point b = nodes[strut.node_b].position;
            const bn::fixed_point mid((a.x() + b.x()) / 2, (a.y() + b.y()) / 2);

            bn::fixed scale = strut.length.safe_division(STRUT_SPRITE_WIDTH);

            if(scale < bn::fixed(1) / 8)
            {
                scale = bn::fixed(1) / 8;
            }

            const bool hot = strut.current_tension * 2 > strut.max_tension;
            bn::sprite_ptr& sprite = _strut_sprites[i];
            sprite.set_position(mid);
            sprite.set_item(bn::sprite_items::strut, hot ? 1 : 0);
            sprite.set_horizontal_scale(scale);
            sprite.set_rotation_angle_safe(strut_angle_deg(a, b));
            sprite.set_visible(_screen == Screen::playing);
        }
    }

    void _update_hud()
    {
        int frame = LABEL_BUILD;

        if(_play_mode == PlayMode::test)
        {
            switch(_bridge.status())
            {
                case bridge::SimStatus::success:
                    frame = LABEL_WIN;
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
    }

    void _animate_train()
    {
        if(!_train.visible())
        {
            return;
        }

        const int chuff_frame = (_frame / 12) % 2;
        _train.set_item(bn::sprite_items::train, chuff_frame);
    }

    bridge::BridgeSystem _bridge;
    Screen _screen = Screen::title;
    PlayMode _play_mode = PlayMode::build;
    bn::fixed_point _cursor_pos;
    bn::optional<int> _selected;
    int _selected_level = 0;
    PauseOption _pause_option = PauseOption::resume;
    int _frame = 0;

    bn::sprite_ptr _cursor;
    bn::sprite_ptr _train;
    bn::sprite_ptr _label;
    bn::sprite_ptr _banner;
    bn::sprite_ptr _banner_ai;
    bn::sprite_ptr _menu_row;
    bn::sprite_ptr _menu_arrow;
    bn::vector<bn::sprite_ptr, bridge::MAX_NODES> _node_sprites;
    bn::vector<bn::sprite_ptr, bridge::MAX_STRUTS> _strut_sprites;
    bn::vector<bn::sprite_ptr, 4> _ground;
    bn::vector<bn::sprite_ptr, 4> _water;
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
