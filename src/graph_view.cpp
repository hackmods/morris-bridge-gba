#include "graph_view.h"

#include "bn_color.h"
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

bn::sprite_palette_ptr& palette_for_band(physics::StressBand band)
{
    static bn::optional<bn::sprite_palette_ptr> green;
    static bn::optional<bn::sprite_palette_ptr> yellow;
    static bn::optional<bn::sprite_palette_ptr> red;
    static bn::optional<bn::sprite_palette_ptr> breaking;
    static bn::sprite_palette_ptr normal =
        bn::sprite_items::strut.palette_item().create_palette();

    auto ensure = [](bn::optional<bn::sprite_palette_ptr>& slot, int r, int g, int b) -> bn::sprite_palette_ptr& {
        if(!slot.has_value())
        {
            slot = bn::sprite_items::strut.palette_item().create_new_palette();
            bn::sprite_palette_ptr& pal = slot.value();
            // bn::color channels are 0–31 (BGR555).
            pal.set_color(5, bn::color(r, g, b));
            pal.set_color(6, bn::color(r + 4 > 31 ? 31 : r + 4, g + 4 > 31 ? 31 : g + 4,
                                       b + 4 > 31 ? 31 : b + 4));
            pal.set_color(12, bn::color(r + 6 > 31 ? 31 : r + 6, g + 2 > 31 ? 31 : g + 2,
                                        b + 2 > 31 ? 31 : b + 2));
        }

        return slot.value();
    };

    switch(band)
    {
        case physics::StressBand::green:
            return ensure(green, 6, 22, 8);
        case physics::StressBand::yellow:
            return ensure(yellow, 26, 22, 4);
        case physics::StressBand::red:
            return ensure(red, 26, 7, 6);
        case physics::StressBand::breaking:
            return ensure(breaking, 31, 8, 28); // flash magenta
        default:
            return normal;
    }
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

int strut_frame_for_stress(bridge::LinkRole role, physics::StressBand band, bool preview)
{
    const bool brace_like = role == bridge::LinkRole::brace || role == bridge::LinkRole::foundation;

    if(band == physics::StressBand::breaking)
    {
        return STRUT_SNAP;
    }

    if(preview)
    {
        return band == physics::StressBand::green ? STRUT_PREVIEW_OK : STRUT_PREVIEW_WEAK;
    }

    if(brace_like)
    {
        return band == physics::StressBand::green ? STRUT_BRACE_IDLE : STRUT_BRACE_HOT;
    }

    return band == physics::StressBand::green ? STRUT_IDLE : STRUT_HOT;
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

    // Glow nodes that touch a highly stressed member.
    bool node_hot[bridge::MAX_NODES] = {};

    if(visible)
    {
        for(const bridge::Strut& strut : bridge.struts())
        {
            if(strut.stress >= bn::fixed(51) / 100)
            {
                if(strut.node_a >= 0 && strut.node_a < bridge::MAX_NODES)
                {
                    node_hot[strut.node_a] = true;
                }

                if(strut.node_b >= 0 && strut.node_b < bridge::MAX_NODES)
                {
                    node_hot[strut.node_b] = true;
                }
            }
        }
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

        if(visible && node_hot[i])
        {
            const bn::fixed pulse = (frame % 16 < 8) ? bn::fixed(1) : bn::fixed(5) / 4;
            sprite.set_scale(pulse);
        }
        else
        {
            sprite.set_scale(1);
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
                            const bool test_mode, const int frame, const bn::fixed preview_load)
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
        bn::fixed stress = strut.stress;

        if(!test_mode)
        {
            // EDIT_MODE: warn by comparing span capacity to the selected engine load.
            stress = bridge::strut_warns_for_load(strut, preview_load) ? bn::fixed(9) / 10
                                                                      : bn::fixed(0);
        }

        const physics::StressBand band = physics::stress_band(stress);
        const int strut_frame = strut_frame_for_stress(strut.role, band, false);
        bn::sprite_ptr& sprite = _strut_sprites[i];
        position_strut_sprite(sprite, a, b, strut_frame);
        sprite.set_palette(palette_for_band(band));

        if(band == physics::StressBand::red || band == physics::StressBand::breaking)
        {
            const bn::fixed pulse = (frame % 16 < 8) ? bn::fixed(1) : bn::fixed(11) / 10;
            sprite.set_horizontal_scale(sprite.horizontal_scale() * pulse);
        }

        sprite.set_visible(visible);
    }
}

} // namespace view
