#include "graph_view.h"

#include "bn_math.h"
#include "bn_optional.h"
#include "bn_sprite_builder.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_utility.h"
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
constexpr int Z_STRUT_PREVIEW = 9;
constexpr int ROD_ART_WIDTH = 6;
constexpr int ROD_SEGMENT_REACH = 24;

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
        palette = bn::sprite_items::strut.palette_item().create_new_palette();
        bn::sprite_palette_ptr& pal = palette.value();
        // bn::color channels are 0–31 (BGR555), not 8-bit sRGB.
        pal.set_color(5, bn::color(22, 6, 5));
        pal.set_color(6, bn::color(26, 7, 6));
        pal.set_color(12, bn::color(27, 11, 7));
    }

    return palette.value();
}

const bn::sprite_palette_ptr& normal_strut_palette()
{
    static bn::sprite_palette_ptr palette =
        bn::sprite_items::strut.palette_item().create_palette();
    return palette;
}

bn::fixed strut_angle_deg(bn::fixed_point a, bn::fixed_point b)
{
    const bn::fixed dx = b.x() - a.x();
    const bn::fixed dy = b.y() - a.y();
    // Screen y grows downward; sprite rotation is counter-clockwise.
    const int yi = (-dy * 16).integer();
    const int xi = (dx * 16).integer();
    bn::fixed angle = bn::degrees_diamond_angle(yi, xi);

    if(angle < 0)
    {
        angle += 360;
    }

    return angle;
}

} // namespace

void LinkStrip::begin()
{
    _sprites_used = 0;
    _mats_used = 0;
}

bn::sprite_ptr* LinkStrip::_next_sprite(const int z_order)
{
    if(_sprites_used == _sprites.size())
    {
        if(_sprites.full())
        {
            return nullptr;
        }

        bn::optional<bn::sprite_ptr> sprite =
            bn::sprite_ptr::create_optional(0, 0, bn::sprite_items::strut);

        if(! sprite)
        {
            return nullptr;
        }

        _sprites.push_back(bn::move(sprite.value()));
    }

    bn::sprite_ptr& sprite = _sprites[_sprites_used++];
    sprite.set_z_order(z_order);
    return &sprite;
}

bn::sprite_affine_mat_ptr* LinkStrip::_next_mat()
{
    if(_mats_used == _mats.size())
    {
        if(_mats.full())
        {
            return nullptr;
        }

        bn::optional<bn::sprite_affine_mat_ptr> mat = bn::sprite_affine_mat_ptr::create_optional();

        if(! mat)
        {
            return nullptr;
        }

        _mats.push_back(bn::move(mat.value()));
    }

    return &_mats[_mats_used++];
}

void LinkStrip::draw(const bn::fixed_point a, const bn::fixed_point b, const int frame,
                     const bool hot, const int z_order)
{
    const bn::fixed span = physics::distance(a, b);

    if(span <= 0)
    {
        return;
    }

    const bn::sprite_palette_ptr& palette = hot ? stress_strut_palette() : normal_strut_palette();
    const bn::fixed dx = physics::abs(b.x() - a.x());
    const bn::fixed dy = physics::abs(b.y() - a.y());
    // Rod art is a 6 px slug in the middle of the tile, drawn to be stretched;
    // timber fills the tile and only needs a matrix when it is steep.
    const bool rod = frame == STRUT_BRACE_IDLE || frame == STRUT_BRACE_HOT;
    bn::sprite_affine_mat_ptr* mat = rod || dy * 4 > dx ? _next_mat() : nullptr;

    if(mat)
    {
        // Stretched segments sharing one matrix, each within the 32 px box a
        // double-size 16 px sprite can show.
        const int reach = rod ? ROD_SEGMENT_REACH : 32;
        const int art_width = rod ? ROD_ART_WIDTH : STRUT_SPRITE_WIDTH;
        const int count = ((span + (reach - 1)) / reach).integer();
        const bn::fixed scale = (span / count).safe_division(art_width);
        mat->set_rotation_angle_safe(strut_angle_deg(a, b));
        mat->set_scale(scale, 1);

        for(int i = 0; i < count; ++i)
        {
            bn::sprite_ptr* sprite = _next_sprite(z_order);

            if(! sprite)
            {
                return;
            }

            const bn::fixed t = bn::fixed(2 * i + 1) / (2 * count);
            sprite->set_affine_mat(*mat);
            sprite->set_position(physics::lerp(a, b, t));
            sprite->set_item(bn::sprite_items::strut, frame);
            sprite->set_palette(palette);
            sprite->set_visible(true);
        }

        return;
    }

    // Shallow (or out of matrices): plain 16 px tiles stepped along the slope,
    // the first and last flush with the joints.
    const int count = span <= STRUT_SPRITE_WIDTH ? 1 : ((span + 15) / 16).integer();
    const bn::fixed half = bn::fixed(STRUT_SPRITE_WIDTH) / 2;

    for(int i = 0; i < count; ++i)
    {
        bn::sprite_ptr* sprite = _next_sprite(z_order);

        if(! sprite)
        {
            return;
        }

        bn::fixed t = bn::fixed(1) / 2;

        if(count > 1)
        {
            t = (half + (span - STRUT_SPRITE_WIDTH) * i / (count - 1)).safe_division(span);
        }

        sprite->remove_affine_mat();
        sprite->set_position(physics::lerp(a, b, t));
        sprite->set_item(bn::sprite_items::strut, frame);
        sprite->set_palette(palette);
        sprite->set_visible(true);
    }
}

void LinkStrip::end(const bool visible)
{
    for(int i = 0; i < _sprites.size(); ++i)
    {
        _sprites[i].set_visible(visible && i < _sprites_used);
    }

    // Segments past this frame's count still hold their matrix; give them up
    // so the matrix itself can be released.
    for(int i = _sprites_used; i < _sprites.size(); ++i)
    {
        _sprites[i].remove_affine_mat();
    }

    while(_mats.size() > _mats_used)
    {
        _mats.pop_back();
    }
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
                            const bool test_mode)
{
    const auto& nodes = bridge.nodes();
    _links.begin();

    if(visible)
    {
        for(const bridge::Strut& strut : bridge.struts())
        {
            const bool hot = test_mode ? bridge::strut_is_strained(strut)
                                        : bridge::strut_was_overworked(strut);
            int strut_frame = hot ? STRUT_HOT : STRUT_IDLE;

            if(strut.role == bridge::LinkRole::brace)
            {
                strut_frame = hot ? STRUT_BRACE_HOT : STRUT_BRACE_IDLE;
            }

            _links.draw(nodes[strut.node_a].position, nodes[strut.node_b].position, strut_frame,
                        hot, Z_STRUT);
        }
    }

    _links.end(visible);
}

void GraphView::show_preview(const bn::fixed_point a, const bn::fixed_point b, const int frame)
{
    _preview.begin();
    _preview.draw(a, b, frame, false, Z_STRUT_PREVIEW);
    _preview.end(true);
}

void GraphView::hide_preview()
{
    _preview.begin();
    _preview.end(false);
}

} // namespace view
