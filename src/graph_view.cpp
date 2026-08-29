#include "graph_view.h"

#include "bn_math.h"
#include "bn_optional.h"
#include "bn_sprite_builder.h"
#include "bn_sprite_items_strut.h"
#include "bn_sprite_items_anchor.h"
#include "bn_sprite_items_node.h"

#include "physics.h"

namespace view
{
namespace
{

constexpr int Z_RING = 4;
constexpr int Z_NODE = 5;
constexpr int Z_STRUT = 10;

bn::sprite_ptr make_sprite(const bn::sprite_item& item, bn::fixed x, bn::fixed y, int z_order)
{
    bn::sprite_builder builder(item);
    builder.set_position(x, y);
    builder.set_z_order(z_order);
    return builder.release_build();
}

const bn::sprite_palette_ptr& stress_strut_palette()
{
    static bn::optional<bn::sprite_palette_ptr> palette;

    if(! palette.has_value())
    {
        palette = bn::sprite_palette_items::strut.create_new();
        bn::sprite_palette_ptr& pal = palette.value();
        pal.set_color(5, bn::color(176, 48, 40));
        pal.set_color(6, bn::color(208, 56, 48));
        pal.set_color(12, bn::color(220, 88, 56));
    }

    return palette.value();
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

} // namespace

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

void GraphView::sync_nodes(const bridge::BridgeSystem& bridge, const bool visible, const int frame,
                           const bn::optional<int>& selected, bn::sprite_ptr& ring)
{
    const auto& nodes = bridge.nodes();

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

        sprite.set_visible(visible);
    }

    if(visible && selected)
    {
        ring.set_position(nodes[selected.value()].position);
        ring.set_visible(true);
        const bn::fixed pulse = (frame % 24 < 12) ? bn::fixed(1) : bn::fixed(9) / 8;
        ring.set_scale(pulse);
    }
    else
    {
        ring.set_visible(false);
    }
}

void GraphView::sync_struts(const bridge::BridgeSystem& bridge, const bool visible,
                            const bool test_mode, const int frame)
{
    const auto& nodes = bridge.nodes();
    const auto& struts = bridge.struts();

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
        const bool hot =
            test_mode && strut.current_tension * 2 > strut.max_tension;
        bn::sprite_ptr& sprite = _strut_sprites[i];
        int strut_frame = STRUT_IDLE;

        if(strut.role == bridge::LinkRole::brace)
        {
            strut_frame = hot ? STRUT_BRACE_HOT : STRUT_BRACE_IDLE;
        }
        else
        {
            strut_frame = hot ? STRUT_HOT : STRUT_IDLE;
        }

        position_strut_sprite(sprite, a, b, strut_frame);

        if(hot)
        {
            sprite.set_palette(stress_strut_palette());
            const bn::fixed pulse = (frame % 16 < 8) ? bn::fixed(1) : bn::fixed(11) / 10;
            sprite.set_horizontal_scale(sprite.horizontal_scale() * pulse);
        }
        else
        {
            sprite.set_palette(bn::sprite_palette_items::strut);
        }

        sprite.set_visible(visible);
    }
}

} // namespace view
