#ifndef MORRIS_BRIDGE_BG_VIEW_H
#define MORRIS_BRIDGE_BG_VIEW_H

/**
 * @file
 * Sky, hills, ground, and water on regular BG; clouds on OAM with parallax.
 */

#include "bn_fixed.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "level.h"

namespace view
{

class BackdropView
{
public:
    void init();

    void set_backdrop_visible(bool visible);
    void set_scenery_visible(bool visible);

    void reset_scenery_layout();
    void layout_for_level(const level::Def& def);

    void animate_clouds();
    void animate_water(int frame);
    void apply_parallax(bn::fixed parallax_shift, const level::Def& def);

private:
    bn::optional<bn::regular_bg_ptr> _sky_bg;
    bn::fixed _sky_base_x = 0;
    bn::fixed _sky_base_y = -48;

    bn::optional<bn::regular_bg_ptr> _hills_bg;
    bn::fixed _hills_base_x = 0;
    bn::fixed _hills_base_y = 24;

    bn::optional<bn::regular_bg_ptr> _ground_bg;
    bn::fixed _ground_base_x = 0;
    bn::fixed _ground_base_y = -64;

    bn::optional<bn::regular_bg_ptr> _water_bg;
    bn::fixed _water_base_x = 0;
    bn::fixed _water_base_y = -84;
    int _water_frame = 0;

    bn::vector<bn::sprite_ptr, 2> _clouds;
};

} // namespace view

#endif
