#ifndef MORRIS_BRIDGE_GRAPH_VIEW_H
#define MORRIS_BRIDGE_GRAPH_VIEW_H

/**
 * @file
 * Diff-synced node and strut sprites for the build/test graph.
 */

#include "bn_fixed_point.h"
#include "bn_optional.h"
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

void position_strut_sprite(bn::sprite_ptr& sprite, bn::fixed_point a, bn::fixed_point b, int frame);

class GraphView
{
public:
    void sync_nodes(const bridge::BridgeSystem& bridge, bool visible, int frame,
                    const bn::optional<int>& selected, bn::sprite_ptr& ring);

    void sync_struts(const bridge::BridgeSystem& bridge, bool visible, bool test_mode, int frame);

private:
    bn::vector<bn::sprite_ptr, bridge::MAX_NODES> _node_sprites;
    bn::vector<bn::sprite_ptr, bridge::MAX_STRUTS> _strut_sprites;
};

} // namespace view

#endif
