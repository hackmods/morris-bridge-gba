#ifndef MORRIS_BRIDGE_GRAPH_VIEW_H
#define MORRIS_BRIDGE_GRAPH_VIEW_H

/**
 * @file
 * Node and link sprites for the build/test graph.
 */

#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_sprite_affine_mat_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "bridge_system.h"

namespace view
{

inline constexpr int STRUT_SPRITE_WIDTH = 16;
inline constexpr int STRUT_IDLE = 0;
inline constexpr int STRUT_HOT = 1;
inline constexpr int STRUT_PREVIEW_OK = 2;
inline constexpr int STRUT_PREVIEW_WEAK = 3;
inline constexpr int STRUT_BRACE_IDLE = 4;
inline constexpr int STRUT_BRACE_HOT = 5;
inline constexpr int STRUT_SNAP = 6;
inline constexpr int STRUT_CABLE = 7;

/// Sprites available to draw every link on screen.
/// Kept under the GBA's 128-sprite ceiling with every other sprite alive.
inline constexpr int MAX_LINK_SPRITES = 56;
/// Affine matrices for steep links (the GBA has 32 in total).
inline constexpr int MAX_LINK_MATS = 26;

/**
 * @brief Draws links as runs of 16 px sprite segments.
 *
 * A GBA affine sprite cannot be scaled past twice its own box, so one
 * stretched sprite clips any link longer than 32 px. Shallow links are tiled
 * with plain sprites stepped along the slope (cheap per scanline, which is
 * where a deck's links all sit); steep links share one affine matrix across
 * segments of at most 32 px.
 *
 * Call begin(), draw() once per link, then end() every frame.
 */
class LinkStrip
{
public:
    void begin();

    void draw(bn::fixed_point a, bn::fixed_point b, int frame, bridge::Heat heat, int z_order);

    /// Hide whatever the frame did not use.
    void end(bool visible);

private:
    [[nodiscard]] bn::sprite_ptr* _next_sprite(int z_order);
    [[nodiscard]] bn::sprite_affine_mat_ptr* _next_mat();

    bn::vector<bn::sprite_ptr, MAX_LINK_SPRITES> _sprites;
    bn::vector<bn::sprite_affine_mat_ptr, MAX_LINK_MATS> _mats;
    int _sprites_used = 0;
    int _mats_used = 0;
};

class GraphView
{
public:
    void sync_nodes(const bridge::BridgeSystem& bridge, bool visible, int frame,
                    const bn::optional<int>& selected, bn::sprite_ptr& ring);

    /**
     * Links are tinted by how hard they work: plain, amber from half their
     * strength, red from four-fifths. Test mode uses the force right now;
     * build mode uses the worst of the last run (a snapped link reads red).
     */
    void sync_struts(const bridge::BridgeSystem& bridge, bool visible, bool test_mode);

    /// One extra link drawn over the graph: the weld ghost, or a snap flash.
    void show_preview(bn::fixed_point a, bn::fixed_point b, int frame);

    void hide_preview();

private:
    bn::vector<bn::sprite_ptr, bridge::MAX_NODES> _node_sprites;
    LinkStrip _links;
    LinkStrip _preview;
};

} // namespace view

#endif
