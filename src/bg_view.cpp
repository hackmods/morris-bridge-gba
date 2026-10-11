#include "bg_view.h"

#include "bn_regular_bg_item.h"
#include "bn_regular_bg_items_bg_ground_0.h"
#include "bn_regular_bg_items_bg_ground_1.h"
#include "bn_regular_bg_items_bg_ground_2.h"
#include "bn_regular_bg_items_bg_hills_0.h"
#include "bn_regular_bg_items_bg_hills_1.h"
#include "bn_regular_bg_items_bg_hills_2.h"
#include "bn_regular_bg_items_bg_sky_0.h"
#include "bn_regular_bg_items_bg_sky_1.h"
#include "bn_regular_bg_items_bg_sky_2.h"
#include "bn_regular_bg_items_bg_terrain_0.h"
#include "bn_regular_bg_items_bg_terrain_1.h"
#include "bn_regular_bg_items_bg_terrain_2.h"
#include "bn_regular_bg_items_bg_terrain_3.h"
#include "bn_regular_bg_items_bg_terrain_4.h"
#include "bn_regular_bg_items_bg_terrain_5.h"
#include "bn_regular_bg_items_bg_terrain_6.h"
#include "bn_regular_bg_items_bg_terrain_7.h"
#include "bn_regular_bg_items_bg_water_0.h"
#include "bn_regular_bg_items_bg_water_1.h"
#include "bn_regular_bg_items_bg_water_2.h"
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

[[nodiscard]] const bn::regular_bg_item& sky_item(const int theme_id)
{
    switch(theme_id)
    {
        case 1:
            return bn::regular_bg_items::bg_sky_1;

        case 2:
            return bn::regular_bg_items::bg_sky_2;

        default:
            return bn::regular_bg_items::bg_sky_0;
    }
}

[[nodiscard]] const bn::regular_bg_item& hills_item(const int theme_id)
{
    switch(theme_id)
    {
        case 1:
            return bn::regular_bg_items::bg_hills_1;

        case 2:
            return bn::regular_bg_items::bg_hills_2;

        default:
            return bn::regular_bg_items::bg_hills_0;
    }
}

[[nodiscard]] const bn::regular_bg_item& ground_item(const int theme_id)
{
    switch(theme_id)
    {
        case 1:
            return bn::regular_bg_items::bg_ground_1;

        case 2:
            return bn::regular_bg_items::bg_ground_2;

        default:
            return bn::regular_bg_items::bg_ground_0;
    }
}

[[nodiscard]] const bn::regular_bg_item& water_item(const int theme_id)
{
    switch(theme_id)
    {
        case 1:
            return bn::regular_bg_items::bg_water_1;

        case 2:
            return bn::regular_bg_items::bg_water_2;

        default:
            return bn::regular_bg_items::bg_water_0;
    }
}

/// Each level's terrain layer is drawn in screen space by tools/gen_graphics.py.
[[nodiscard]] const bn::regular_bg_item& terrain_item(const int level_index)
{
    switch(level_index)
    {
        case 1:
            return bn::regular_bg_items::bg_terrain_1;

        case 2:
            return bn::regular_bg_items::bg_terrain_2;

        case 3:
            return bn::regular_bg_items::bg_terrain_3;

        case 4:
            return bn::regular_bg_items::bg_terrain_4;

        case 5:
            return bn::regular_bg_items::bg_terrain_5;

        case 6:
            return bn::regular_bg_items::bg_terrain_6;

        case 7:
            return bn::regular_bg_items::bg_terrain_7;

        default:
            return bn::regular_bg_items::bg_terrain_0;
    }
}

constexpr bn::fixed SKY_BASE_Y = -48;
constexpr int GROUND_STRIP_CENTER_Y = 248;
/// Bitmap row of the river surface in the water layer (WATER_BAND_TOP).
constexpr int WATER_SURFACE_ROW = 200;

} // namespace

void BackdropView::init()
{
    apply_theme(0);

    _clouds.push_back(bn::sprite_items::cloud.create_sprite(-80, -64));
    _clouds.push_back(bn::sprite_items::cloud.create_sprite(24, -72));
    _clouds.back().set_horizontal_scale(bn::fixed(3) / 2);

    for(bn::sprite_ptr& cloud : _clouds)
    {
        cloud.set_z_order(25);
    }
}

void BackdropView::apply_theme(const int bg_theme_id)
{
    const int theme_id = (bg_theme_id >= 0 && bg_theme_id <= 2) ? bg_theme_id : 0;

    if(theme_id == _active_theme_id)
    {
        return;
    }

    _active_theme_id = theme_id;
    _water_frame = 0;

    const bn::regular_bg_item& sky = sky_item(theme_id);
    const bn::regular_bg_item& hills = hills_item(theme_id);
    const bn::regular_bg_item& ground = ground_item(theme_id);
    const bn::regular_bg_item& water = water_item(theme_id);

    if(! _sky_bg)
    {
        _sky_bg = sky.create_bg(_sky_base_x, _sky_base_y);
        // Every layer shares the back priority so sprites (track, joints,
        // train, HUD) always draw in front; z-order sorts the layers.
        _sky_bg->set_priority(3);
        _sky_bg->set_z_order(3);
    }
    else
    {
        _sky_bg->set_item(sky);
    }

    if(! _hills_bg)
    {
        _hills_bg = hills.create_bg(_hills_base_x, _hills_base_y);
        _hills_bg->set_priority(3);
        _hills_bg->set_z_order(2);
    }
    else
    {
        _hills_bg->set_item(hills);
    }

    if(! _ground_bg)
    {
        _ground_bg = ground.create_bg(_ground_base_x, _ground_base_y);
        _ground_bg->set_priority(3);
        _ground_bg->set_z_order(0);
    }
    else
    {
        _ground_bg->set_item(ground);
    }

    if(! _water_bg)
    {
        _water_bg = water.create_bg(_water_base_x, _water_base_y);
        _water_bg->set_priority(3);
        _water_bg->set_z_order(1);
    }
    else
    {
        _water_bg->set_item(water, 0);
    }

    for(bn::sprite_ptr& cloud : _clouds)
    {
        cloud.set_visible(theme_id != 2);
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
        if(_active_theme_id != 2)
        {
            cloud.set_visible(visible);
        }
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
    apply_theme(0);
    _hills_base_y = 24;
    _sky_base_y = SKY_BASE_Y;

    if(_sky_bg)
    {
        _sky_bg->set_y(_sky_base_y);
    }

    if(_hills_bg)
    {
        _hills_bg->set_y(_hills_base_y);
    }

    // Menus use the generic bank strip, not any level's terrain.
    if(_terrain_level >= 0 && _ground_bg)
    {
        _ground_bg->set_item(ground_item(0));
    }

    _terrain_level = -1;

    const bn::fixed bank_ground_y = game::GROUND_TILE_Y;
    const bn::fixed water_y = bank_ground_y - 2;
    _ground_base_y = bg_center_for_screen_y(bank_ground_y, GROUND_STRIP_CENTER_Y);
    _water_base_y = bg_center_for_screen_y(water_y, WATER_SURFACE_ROW);

    if(_ground_bg)
    {
        _ground_bg->set_y(_ground_base_y);
    }

    if(_water_bg)
    {
        _water_bg->set_y(_water_base_y);
    }
}

void BackdropView::layout_for_level(const level::Def& def, const int level_index)
{
    apply_theme(def.bg_theme_id);
    _terrain_level = level_index;

    if(_ground_bg)
    {
        _ground_bg->set_item(terrain_item(level_index));
    }

    // The art is composed around a deck at TRACK_Y; raise the whole landscape
    // with the line so a high deck still sits on its horizon.
    const bn::fixed lift = def.deck_y - game::TRACK_Y;
    _hills_base_y = hill_y_for_theme(def.bg_theme_id) + lift;
    _sky_base_y = SKY_BASE_Y + lift;

    if(_sky_bg)
    {
        _sky_bg->set_y(_sky_base_y);
    }

    if(_hills_bg)
    {
        _hills_bg->set_y(_hills_base_y);
    }

    // Terrain is authored in screen space: no offset, and no parallax either,
    // because the banks have to stay under the anchors.
    _ground_base_y = 0;

    if(_ground_bg)
    {
        _ground_bg->set_position(_ground_base_x, _ground_base_y);
    }

    _water_base_y = bg_center_for_screen_y(def.water_y, WATER_SURFACE_ROW);

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
    if(! _water_bg || _active_theme_id < 0)
    {
        return;
    }

    const int water_frame = (frame / 16) % 2;

    if(water_frame != _water_frame)
    {
        _water_frame = water_frame;
        _water_bg->set_map(water_item(_active_theme_id).map_item(), _water_frame);
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

    // In a level the ground and river are the level itself and hold still.
    const bool fixed_terrain = _terrain_level >= 0;

    if(_ground_bg)
    {
        _ground_bg->set_x(fixed_terrain ? _ground_base_x : _ground_base_x - front_shift / 3);
        _ground_bg->set_y(_ground_base_y);
    }

    if(_water_bg)
    {
        _water_bg->set_x(fixed_terrain ? _water_base_x : _water_base_x - front_shift / 2);
        _water_bg->set_y(_water_base_y);
    }
}

} // namespace view
