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
#include "bn_sprite_items_digit.h"
#include "bn_sprite_items_ghost.h"
#include "bn_sprite_items_ground.h"
#include "bn_sprite_items_label.h"
#include "bn_sprite_items_node.h"
#include "bn_sprite_items_panel.h"
#include "bn_sprite_items_ring.h"
#include "bn_sprite_items_sky.h"
#include "bn_sprite_items_strut.h"
#include "bn_sprite_items_toast.h"
#include "bn_sprite_items_train.h"
#include "bn_sprite_items_water.h"

#include "bridge_system.h"
#include "physics.h"

namespace
{

constexpr bn::fixed CURSOR_SPEED = 2;
constexpr bn::fixed CURSOR_FAST = 4;
constexpr bn::fixed STRUT_SPRITE_WIDTH = 16;
constexpr bn::fixed GRID_STEP = 4;
constexpr int TOAST_FRAMES = 90;

constexpr int LABEL_BUILD = 0;
constexpr int LABEL_TEST = 1;
constexpr int LABEL_FAIL = 2;
constexpr int LABEL_WIN = 3;
constexpr int LABEL_NO_GO = 4;
constexpr int LABEL_FULL = 5;
constexpr int LABEL_UNDO = 6;

constexpr int TOAST_DUP = 0;
constexpr int TOAST_WEAK = 1;
constexpr int TOAST_DEL = 2;

constexpr int STRUT_IDLE = 0;
constexpr int STRUT_HOT = 1;
constexpr int STRUT_PREVIEW_OK = 2;
constexpr int STRUT_PREVIEW_WEAK = 3;

constexpr int Z_CURSOR = -32767;
constexpr int Z_TOAST = -18;
constexpr int Z_LABEL = -20;
constexpr int Z_HUD = -22;
constexpr int Z_TRAIN = -10;
constexpr int Z_RING = 4;
constexpr int Z_NODE = 5;
constexpr int Z_GHOST = 6;
constexpr int Z_STRUT = 10;
constexpr int Z_STRUT_PREVIEW = 9;
constexpr int Z_GROUND = 20;
constexpr int Z_WATER = 21;
constexpr int Z_SKY = 25;

enum class ToastKind
{
    none,
    dup,
    weak,
    deleted,
    full,
    undo,
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

bn::fixed snap_grid(bn::fixed value)
{
    const int raw = value.shift_integer();
    const int snapped = ((raw + 2) / 4) * 4;
    return bn::fixed(snapped);
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

void position_strut_sprite(bn::sprite_ptr& sprite, bn::fixed_point a, bn::fixed_point b, int frame)
{
    const bn::fixed_point mid((a.x() + b.x()) / 2, (a.y() + b.y()) / 2);
    bn::fixed scale = physics::distance(a, b).safe_division(STRUT_SPRITE_WIDTH);

    if(scale < bn::fixed(1) / 8)
    {
        scale = bn::fixed(1) / 8;
    }

    sprite.set_position(mid);
    sprite.set_item(bn::sprite_items::strut, frame);
    sprite.set_horizontal_scale(scale);
    sprite.set_rotation_angle_safe(strut_angle_deg(a, b));
    sprite.set_visible(true);
}

class Game
{
public:
    Game() :
        _cursor(make_sprite(bn::sprite_items::cursor, 0, 0, Z_CURSOR)),
        _train(make_sprite(bn::sprite_items::train, _bridge.train_position().x(), _bridge.train_position().y(), Z_TRAIN)),
        _label(make_sprite(bn::sprite_items::label, -96, -72, Z_LABEL)),
        _ghost(make_sprite(bn::sprite_items::ghost, 0, 0, Z_GHOST)),
        _ring(make_sprite(bn::sprite_items::ring, 0, 0, Z_RING)),
        _preview_strut(make_sprite(bn::sprite_items::strut, 0, 0, Z_STRUT_PREVIEW)),
        _toast(make_sprite(bn::sprite_items::toast, 0, 64, Z_TOAST))
    {
        bn::bg_palettes::set_transparent_color(bn::color(8, 14, 20));

        _train.set_visible(false);
        _ghost.set_visible(false);
        _ring.set_visible(false);
        _preview_strut.set_visible(false);
        _toast.set_visible(false);
        _label.set_item(bn::sprite_items::label, LABEL_BUILD);

        _place_scenery();
        _layout_hud();
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
        _update_preview();
        _update_hud();
        _update_toast();
        _animate_scenery();
    }

private:
    enum class Mode
    {
        build,
        test
    };

    void _place_scenery()
    {
        for(int x = -112; x <= 80; x += 32)
        {
            _sky.push_back(make_sprite(bn::sprite_items::sky, x, -64, Z_SKY));
        }

        for(int x = -112; x <= 80; x += 32)
        {
            _ground.push_back(make_sprite(bn::sprite_items::ground, x, 56, Z_GROUND));
        }

        for(int x = -112; x <= 80; x += 32)
        {
            bn::sprite_ptr tile = make_sprite(bn::sprite_items::water, x, 74, Z_WATER);
            tile.set_item(bn::sprite_items::water, 0);
            _water.push_back(tile);
        }
    }

    void _layout_hud()
    {
        _hud_panels.push_back(make_sprite(bn::sprite_items::panel, -112, -72, Z_HUD));
        _hud_panels.push_back(make_sprite(bn::sprite_items::panel, -80, -72, Z_HUD));
        _hud_panels.push_back(make_sprite(bn::sprite_items::panel, -48, -72, Z_HUD));

        _node_count_digits.push_back(make_sprite(bn::sprite_items::digit, -88, -72, Z_HUD));
        _node_count_digits.push_back(make_sprite(bn::sprite_items::digit, -80, -72, Z_HUD));

        _strut_count_digits.push_back(make_sprite(bn::sprite_items::digit, -40, -72, Z_HUD));
        _strut_count_digits.push_back(make_sprite(bn::sprite_items::digit, -32, -72, Z_HUD));
    }

    void _set_counter(bn::vector<bn::sprite_ptr, 2>& digits, int value, bn::fixed x_base)
    {
        const int clamped = value < 0 ? 0 : (value > 99 ? 99 : value);
        const int tens = clamped / 10;
        const int ones = clamped % 10;

        digits[0].set_item(bn::sprite_items::digit, tens);
        digits[0].set_position(x_base, -72);
        digits[0].set_visible(true);

        digits[1].set_item(bn::sprite_items::digit, ones);
        digits[1].set_position(x_base + 8, -72);
        digits[1].set_visible(true);
    }

    void _show_toast(ToastKind kind)
    {
        _toast_kind = kind;
        _toast_timer = TOAST_FRAMES;
        _toast.set_visible(true);

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

            case ToastKind::full:
                _label.set_item(bn::sprite_items::label, LABEL_FULL);
                break;

            case ToastKind::undo:
                _label.set_item(bn::sprite_items::label, LABEL_UNDO);
                break;

            default:
                break;
        }
    }

    void _update_toast()
    {
        if(_toast_timer <= 0)
        {
            _toast.set_visible(false);
            _toast_kind = ToastKind::none;
            return;
        }

        --_toast_timer;

        if(_toast_kind == ToastKind::full || _toast_kind == ToastKind::undo)
        {
            return;
        }

        _toast.set_visible(true);
    }

    void _animate_scenery()
    {
        const int frame = (_frame / 20) % 2;

        for(bn::sprite_ptr& tile : _water)
        {
            tile.set_item(bn::sprite_items::water, frame);
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
        _cursor_pos.set_y(clamp(_cursor_pos.y(), -60, 64));

        const bn::optional<int> hovered = _bridge.find_node_near(_cursor_pos);
        const bn::fixed_point draw_at = hovered ? _bridge.nodes()[hovered.value()].position : _cursor_pos;
        _cursor.set_position(draw_at);
        _cursor.set_visible(true);

        if(!hovered)
        {
            _ghost.set_position(_cursor_pos);
            _ghost.set_visible(true);
        }
        else
        {
            _ghost.set_visible(false);
        }

        if(bn::keypad::a_pressed())
        {
            _handle_confirm(hovered);
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
                _show_toast(ToastKind::undo);
            }
            else if(undone == bridge::UndoResult::removed_strut)
            {
                _show_toast(ToastKind::undo);
            }
        }

        if(bn::keypad::select_pressed())
        {
            _bridge.reset_level();
            _selected.reset();
            _toast_timer = 0;
            _toast_kind = ToastKind::none;
        }

        if(bn::keypad::start_pressed())
        {
            _enter_test();
        }
    }

    void _handle_confirm(bn::optional<int> hovered)
    {
        if(hovered && _selected && _selected.value() == hovered.value())
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

            const bn::fixed snapped_x = snap_grid(_cursor_pos.x());
            const bn::fixed snapped_y = snap_grid(_cursor_pos.y());
            const bn::optional<int> created = _bridge.add_node(snapped_x, snapped_y, false);

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
                {
                    if(preview.max_tension < physics::BASE_WEIGHT)
                    {
                        _show_toast(ToastKind::weak);
                    }

                    (void)_bridge.add_strut(_selected.value(), node_id);
                    break;
                }

                default:
                    break;
            }
        }

        _selected = node_id;
    }

    void _enter_test()
    {
        _mode = Mode::test;
        _selected.reset();
        _cursor.set_visible(false);
        _ghost.set_visible(false);
        _ring.set_visible(false);
        _preview_strut.set_visible(false);
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

            sprite.set_visible(true);
        }

        if(_mode == Mode::build && _selected)
        {
            const bn::fixed_point pos = nodes[_selected.value()].position;
            _ring.set_position(pos);
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
            const bool hot = _mode == Mode::test &&
                strut.current_tension * 2 > strut.max_tension;
            bn::sprite_ptr& sprite = _strut_sprites[i];
            position_strut_sprite(sprite, a, b, hot ? STRUT_HOT : STRUT_IDLE);
        }
    }

    void _update_preview()
    {
        if(_mode != Mode::build || !_selected)
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
            to = bn::fixed_point(snap_grid(_cursor_pos.x()), snap_grid(_cursor_pos.y()));
            const bn::fixed length = physics::distance(from, to);

            if(length > 0)
            {
                max_tension = physics::max_tension_for_length(length);
                show = true;
            }
        }

        if(show)
        {
            const int frame = max_tension < physics::BASE_WEIGHT ?
                STRUT_PREVIEW_WEAK : STRUT_PREVIEW_OK;
            position_strut_sprite(_preview_strut, from, to, frame);
            return;
        }

        _preview_strut.set_visible(false);
    }

    void _update_hud()
    {
        _set_counter(_node_count_digits, _bridge.player_node_count(), -88);
        _set_counter(_strut_count_digits, _bridge.struts().size(), -40);

        if(_toast_timer > 0 &&
           (_toast_kind == ToastKind::full || _toast_kind == ToastKind::undo))
        {
            return;
        }

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
    int _toast_timer = 0;
    ToastKind _toast_kind = ToastKind::none;

    bn::sprite_ptr _cursor;
    bn::sprite_ptr _train;
    bn::sprite_ptr _label;
    bn::sprite_ptr _ghost;
    bn::sprite_ptr _ring;
    bn::sprite_ptr _preview_strut;
    bn::sprite_ptr _toast;
    bn::vector<bn::sprite_ptr, bridge::MAX_NODES> _node_sprites;
    bn::vector<bn::sprite_ptr, bridge::MAX_STRUTS> _strut_sprites;
    bn::vector<bn::sprite_ptr, 8> _sky;
    bn::vector<bn::sprite_ptr, 8> _ground;
    bn::vector<bn::sprite_ptr, 8> _water;
    bn::vector<bn::sprite_ptr, 3> _hud_panels;
    bn::vector<bn::sprite_ptr, 2> _node_count_digits;
    bn::vector<bn::sprite_ptr, 2> _strut_count_digits;
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
