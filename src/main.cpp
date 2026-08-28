#include "bn_core.h"
#include "bn_color.h"
#include "bn_bg_palettes.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_optional.h"
#include "bn_sprite_builder.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "bn_sprite_items_anchor.h"
#include "bn_sprite_items_cursor.h"
#include "bn_sprite_items_ground.h"
#include "bn_sprite_items_label.h"
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

constexpr int Z_CURSOR = -32767;
constexpr int Z_LABEL = -20;
constexpr int Z_TRAIN = -10;
constexpr int Z_NODE = 5;
constexpr int Z_STRUT = 10;
constexpr int Z_GROUND = 20;
constexpr int Z_WATER = 21;

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

bn::fixed strut_angle_deg(bn::fixed_point a, bn::fixed_point b)
{
    const bn::fixed dx = b.x() - a.x();
    const bn::fixed dy = b.y() - a.y();

    // Diamond-angle approximation (no BIOS atan2). Scale to keep integer range.
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
        _cursor(make_sprite(bn::sprite_items::cursor, 0, 0, Z_CURSOR)),
        _train(make_sprite(bn::sprite_items::train, _bridge.train_position().x(), _bridge.train_position().y(), Z_TRAIN)),
        _label(make_sprite(bn::sprite_items::label, -104, -72, Z_LABEL))
    {
        bn::bg_palettes::set_transparent_color(bn::color(8, 14, 20));

        _train.set_visible(false);
        _label.set_item(bn::sprite_items::label, LABEL_BUILD);

        _place_scenery();
        _sync_node_sprites();
        _sync_strut_sprites();
    }

    void update()
    {
        ++_frame;

        if(_mode == Mode::build)
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

private:
    enum class Mode
    {
        build,
        test
    };

    void _place_scenery()
    {
        const int ground_x[] = { -104, -72, 72, 104 };
        const int water_x[] = { -40, -8, 24, 56 };

        for(int x : ground_x)
        {
            _ground.push_back(make_sprite(bn::sprite_items::ground, x, 56, Z_GROUND));
        }

        for(int x : water_x)
        {
            _water.push_back(make_sprite(bn::sprite_items::water, x, 74, Z_WATER));
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
            _bridge.reset_level();
            _selected.reset();
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
        _mode = Mode::test;
        _selected.reset();
        _cursor.set_visible(false);
        _train.set_visible(true);
        (void)_bridge.begin_test();
        _train.set_position(_bridge.train_position());
    }

    void _leave_test()
    {
        _bridge.end_test();
        _mode = Mode::build;
        _train.set_visible(false);
        _cursor.set_visible(true);
        _cursor.set_position(_cursor_pos);
    }

    void _update_test()
    {
        (void)_bridge.update();
        _train.set_position(_bridge.train_position());
        _train.set_visible(_train.position().y() < bridge::FALL_Y_LIMIT);

        if(bn::keypad::start_pressed() || bn::keypad::select_pressed() ||
           bn::keypad::b_pressed())
        {
            _leave_test();

            if(bn::keypad::select_pressed())
            {
                _bridge.reset_level();
            }
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

            const bool selected = _selected && _selected.value() == i;
            const bool blink_off = selected && ((_frame / 8) % 2 == 0);
            sprite.set_visible(!blink_off);
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
            sprite.set_visible(true);
        }
    }

    void _update_hud()
    {
        int frame = LABEL_BUILD;

        if(_mode == Mode::test)
        {
            switch(_bridge.status())
            {
                case bridge::SimStatus::success:
                    frame = LABEL_WIN;
                    break;

                case bridge::SimStatus::failed_fall:
                    frame = LABEL_FAIL;
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

    bridge::BridgeSystem _bridge;
    Mode _mode = Mode::build;
    bn::fixed_point _cursor_pos;
    bn::optional<int> _selected;
    int _frame = 0;

    bn::sprite_ptr _cursor;
    bn::sprite_ptr _train;
    bn::sprite_ptr _label;
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
