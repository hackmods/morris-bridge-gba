#include "bg_view.h"

#include "bn_regular_bg_items_bg_ground.h"
#include "bn_regular_bg_items_bg_hills.h"
#include "bn_regular_bg_items_bg_sky.h"
#include "bn_regular_bg_items_bg_water.h"
#include "bn_sprite_items_cloud.h"

#include "game_constants.h"

namespace view
{
namespace
{

/// BG center Y so @p bmp_y lands on @p screen_y.
[[nodiscard]] bn::fixed bg_center_for_screen_y(const bn::fixed screen_y, const int bmp_y)
{
    return screen_y - bn::fixed(bmp_y - 128);
}

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

constexpr int GROUND_STRIP_CENTER_Y = 248;
constexpr int WATER_STRIP_CENTER_Y = 240;

} // namespace

void BackdropView::init()
{
    _sky_bg = bn::regular_bg_items::bg_sky.create_bg(_sky_base_x, _sky_base_y);
    _sky_bg->set_priority(3);

    _hills_bg = bn::regular_bg_items::bg_hills.create_bg(_hills_base_x, _hills_base_y);
    _hills_bg->set_priority(2);

    _ground_bg = bn::regular_bg_items::bg_ground.create_bg(_ground_base_x, _ground_base_y);
    _ground_bg->set_priority(1);

    _water_bg = bn::regular_bg_items::bg_water.create_bg(_water_base_x, _water_base_y);
    _water_bg->set_priority(1);

    _clouds.push_back(bn::sprite_items::cloud.create_sprite(-80, -64));
    _clouds.push_back(bn::sprite_items::cloud.create_sprite(24, -72));
    _clouds.back().set_horizontal_scale(bn::fixed(3) / 2);

    for(bn::sprite_ptr& cloud : _clouds)
    {
        cloud.set_z_order(25);
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

    if(_ground_bg)
    {
        _ground_bg->set_visible(visible);
    }

    if(_water_bg)
    {
        _water_bg->set_visible(visible);
    }
}

void BackdropView::reset_scenery_layout()
{
    _hills_base_y = 24;

    if(_hills_bg)
    {
        _hills_bg->set_y(_hills_base_y);
    }

    const bn::fixed bank_ground_y = game::GROUND_TILE_Y;
    const bn::fixed water_y = bank_ground_y + 6;
    _ground_base_y = bg_center_for_screen_y(bank_ground_y, GROUND_STRIP_CENTER_Y);
    _water_base_y = bg_center_for_screen_y(water_y, WATER_STRIP_CENTER_Y);

    if(_ground_bg)
    {
        _ground_bg->set_y(_ground_base_y);
    }

    if(_water_bg)
    {
        _water_bg->set_y(_water_base_y);
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

    _ground_base_y = bg_center_for_screen_y(bank_ground_y, GROUND_STRIP_CENTER_Y);

    if(_ground_bg)
    {
        _ground_bg->set_y(_ground_base_y);
    }

    _water_base_y = bg_center_for_screen_y(water_y, WATER_STRIP_CENTER_Y);

    if(_water_bg)
    {
        _water_bg->set_y(_water_base_y);
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
    if(! _water_bg)
    {
        return;
    }

    const int water_frame = (frame / 16) % 2;

    if(water_frame != _water_frame)
    {
        _water_frame = water_frame;
        _water_bg->set_map(bn::regular_bg_items::bg_water.map_item(), _water_frame);
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

    if(_ground_bg)
    {
        _ground_bg->set_x(_ground_base_x - front_shift / 3);
        _ground_bg->set_y(_ground_base_y);
    }

    if(_water_bg)
    {
        _water_bg->set_x(_water_base_x - front_shift / 2);
        _water_bg->set_y(_water_base_y);
    }
}

} // namespace view
