#include "bg_view.h"

#include "bn_regular_bg_items_bg_hills.h"
#include "bn_regular_bg_items_bg_sky.h"
#include "bn_sprite_items_cloud.h"
#include "bn_sprite_items_ground.h"
#include "bn_sprite_items_water.h"

#include "game_constants.h"

namespace view
{
namespace
{

[[nodiscard]] bn::fixed hill_y_for_theme(const int bg_theme_id)
{
    switch(bg_theme_id)
    {
        case 1:
            return 18;

        case 2:
            return 28;

        default:
            return 24;
    }
}

} // namespace

void BackdropView::init()
{
    _sky_bg = bn::regular_bg_items::bg_sky.create_bg(_sky_base_x, _sky_base_y);
    _sky_bg->set_priority(3);

    _hills_bg = bn::regular_bg_items::bg_hills.create_bg(_hills_base_x, _hills_base_y);
    _hills_bg->set_priority(2);

    _clouds.push_back(bn::sprite_items::cloud.create_sprite(-80, -64));
    _clouds.push_back(bn::sprite_items::cloud.create_sprite(24, -72));
    _clouds.back().set_horizontal_scale(bn::fixed(3) / 2);

    for(bn::sprite_ptr& cloud : _clouds)
    {
        cloud.set_z_order(25);
    }

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

    for(int x : water_x)
    {
        bn::sprite_ptr tile = bn::sprite_items::water.create_sprite(x, 62);
        tile.set_z_order(21);
        _water.push_back(tile);
        _water_base_x.push_back(x);
        _water_base_y.push_back(62);
    }
}

void BackdropView::set_backdrop_visible(const bool visible)
{
    if(_sky_bg)
    {
        _sky_bg->set_visible(visible);
    }

    for(bn::sprite_ptr& cloud : _clouds)
    {
        cloud.set_visible(visible);
    }
}

void BackdropView::set_scenery_visible(const bool visible)
{
    if(_hills_bg)
    {
        _hills_bg->set_visible(visible);
    }

    for(bn::sprite_ptr& tile : _ground)
    {
        tile.set_visible(visible);
    }

    for(bn::sprite_ptr& tile : _water)
    {
        tile.set_visible(visible);
    }
}

void BackdropView::reset_scenery_layout()
{
    _hills_base_y = 24;

    if(_hills_bg)
    {
        _hills_bg->set_y(_hills_base_y);
    }

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
}

void BackdropView::layout_for_level(const level::Def& def)
{
    const bn::fixed bank_ground_y = def.deck_y + (game::GROUND_TILE_Y - game::TRACK_Y);
    const bn::fixed water_y = bank_ground_y + 6;
    _hills_base_y = hill_y_for_theme(def.bg_theme_id);

    if(_hills_bg)
    {
        _hills_bg->set_y(_hills_base_y);
    }

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
}

void BackdropView::animate_clouds()
{
    for(bn::sprite_ptr& cloud : _clouds)
    {
        bn::fixed x = cloud.position().x() + bn::fixed(1) / 8;

        if(x > 130)
        {
            x = -130;
        }

        cloud.set_x(x);
    }
}

void BackdropView::animate_water(const int frame)
{
    const int water_frame = (frame / 16) % 2;

    for(bn::sprite_ptr& tile : _water)
    {
        tile.set_item(bn::sprite_items::water, water_frame);
    }
}

void BackdropView::apply_parallax(const bn::fixed parallax_shift, const level::Def& def)
{
    const bn::fixed back_shift = parallax_shift.safe_multiplication(def.parallax_back);
    const bn::fixed front_shift = parallax_shift;

    if(_sky_bg)
    {
        _sky_bg->set_x(_sky_base_x - back_shift / 4);
        _sky_bg->set_y(_sky_base_y);
    }

    if(_hills_bg)
    {
        _hills_bg->set_x(_hills_base_x - back_shift / 2);
        _hills_bg->set_y(_hills_base_y);
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

} // namespace view
