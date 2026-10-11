#ifndef MORRIS_BRIDGE_LEVEL_SOLUTIONS_H
#define MORRIS_BRIDGE_LEVEL_SOLUTIONS_H

/**
 * @file
 * Reference bridge layouts used by host playthrough tests.
 *
 * Each design is the par solution for its level: the joints the player adds
 * (in order, so they follow the level's seeded anchors in the node list) and
 * the links between node indices. They are proof that every level is
 * solvable at par with its scheduled engine — nothing in the ROM reads them.
 */

#include "bridge_system.h"
#include "engine.h"
#include "level.h"

namespace level_solutions
{

struct Joint
{
    int x;
    int y;
};

struct Link
{
    int a;
    int b;
    bridge::LinkRole role;
};

struct Design
{
    const Joint* joints;
    int joint_count;
    const Link* links;
    int link_count;
};

namespace detail
{

inline constexpr bridge::LinkRole T = bridge::LinkRole::track;
inline constexpr bridge::LinkRole R = bridge::LinkRole::brace;

// Knapford — anchors: 0 start, 1 end, 2 riverbed rock.
// One deck joint over the rock, propped by a timber post.
inline constexpr Joint KNAPFORD_JOINTS[] = { { 0, 12 } };
inline constexpr Link KNAPFORD_LINKS[] = { { 0, 3, T }, { 3, 1, T }, { 2, 3, T } };

// Gordon's Hill — anchors: 0 start, 1 end, 2 left crag, 3 right crag.
// The deck climbs 32 px; every joint hangs from a crag on a rod.
inline constexpr Joint GORDON_HILL_JOINTS[] = { { -40, 36 }, { 0, 28 }, { 40, 20 } };
inline constexpr Link GORDON_HILL_LINKS[] = {
    { 0, 4, T }, { 4, 5, T }, { 5, 6, T }, { 6, 1, T },
    { 2, 4, R }, { 2, 5, R }, { 3, 6, R },
};

// Culdee Fell — anchors: 0 start, 1 end. A Warren through-truss: rods for the
// end posts and top chord (compression), timber for the inner diagonals.
inline constexpr Joint CULDEE_JOINTS[] = {
    { -20, 30 }, { 20, 30 }, { -40, 0 }, { 0, 0 }, { 40, 0 },
};
inline constexpr Link CULDEE_LINKS[] = {
    { 0, 2, T }, { 2, 3, T }, { 3, 1, T },
    { 0, 4, R }, { 4, 2, T }, { 2, 5, T }, { 5, 3, T }, { 3, 6, T }, { 6, 1, R },
    { 4, 5, R }, { 5, 6, R },
};

// Tidmouth — anchors: 0 start, 1 end. Timber queen-post truss; the one
// diagonal stops the middle panel racking when the engine is off-centre.
inline constexpr Joint TIDMOUTH_JOINTS[] = { { -16, 30 }, { 16, 30 }, { -16, 2 }, { 16, 2 } };
inline constexpr Link TIDMOUTH_LINKS[] = {
    { 0, 2, T }, { 2, 3, T }, { 3, 1, T },
    { 0, 4, T }, { 4, 2, T }, { 4, 5, T }, { 5, 3, T }, { 5, 1, T }, { 4, 3, T },
};

// Wellsworth — anchors: 0 start, 1 end, 2 left ledge, 3 right ledge.
// Each deck joint is strutted up from the ledge below it.
inline constexpr Joint WELLSWORTH_JOINTS[] = { { -20, 0 }, { 20, 0 } };
inline constexpr Link WELLSWORTH_LINKS[] = {
    { 0, 4, T }, { 4, 5, T }, { 5, 1, T }, { 2, 4, R }, { 3, 5, R },
};

// Crovan's Gate — anchors: 0 start, 1 end, 2 river rock.
// A three-legged trestle on the rock, then a king-post truss over open water.
inline constexpr Joint CROVAN_JOINTS[] = {
    { -52, 0 }, { -16, 0 }, { 20, 0 }, { 54, 0 }, { 54, -32 },
};
inline constexpr Link CROVAN_LINKS[] = {
    { 0, 3, T }, { 3, 4, T }, { 4, 5, T }, { 5, 6, T }, { 6, 1, T },
    { 2, 3, R }, { 2, 4, T }, { 2, 5, R },
    { 5, 7, R }, { 7, 1, R }, { 7, 6, R },
};

// Callan — anchors: 0 start, 1 end, 2 left tower, 3 right tower.
// Stays from the castle towers hold the outer joints; a rod king post between them
// carries the middle one.
inline constexpr Joint CALLAN_JOINTS[] = { { -36, 40 }, { 0, 40 }, { 36, 40 }, { 0, 4 } };
inline constexpr Link CALLAN_LINKS[] = {
    { 0, 4, T }, { 4, 5, T }, { 5, 6, T }, { 6, 1, T },
    { 2, 4, R }, { 3, 6, R }, { 4, 7, R }, { 6, 7, R }, { 7, 5, R },
};

// Vicarstown — anchors: 0 start, 1 end, 2 pier in the channel.
// Steep rod legs off the pier, then a rod king-post truss on each side.
inline constexpr Joint VICARSTOWN_JOINTS[] = {
    { -48, 8 }, { -16, 8 }, { 16, 8 }, { 48, 8 }, { -48, -28 }, { 48, -28 },
};
inline constexpr Link VICARSTOWN_LINKS[] = {
    { 0, 3, T }, { 3, 4, T }, { 4, 5, T }, { 5, 6, T }, { 6, 1, T },
    { 2, 4, R }, { 2, 5, R },
    { 0, 7, R }, { 7, 4, R }, { 5, 8, R }, { 8, 1, R }, { 7, 3, R }, { 8, 6, R },
};

#define MORRIS_DESIGN(name) \
    { name##_JOINTS, int(sizeof(name##_JOINTS) / sizeof(Joint)), \
      name##_LINKS, int(sizeof(name##_LINKS) / sizeof(Link)) }

inline constexpr Design DESIGNS[level::COUNT] = {
    MORRIS_DESIGN(KNAPFORD),
    MORRIS_DESIGN(GORDON_HILL),
    MORRIS_DESIGN(CULDEE),
    MORRIS_DESIGN(TIDMOUTH),
    MORRIS_DESIGN(WELLSWORTH),
    MORRIS_DESIGN(CROVAN),
    MORRIS_DESIGN(CALLAN),
    MORRIS_DESIGN(VICARSTOWN),
};

#undef MORRIS_DESIGN

} // namespace detail

[[nodiscard]] inline const Design& reference(const int level_index)
{
    return detail::DESIGNS[level_index];
}

/**
 * @brief Weld a design onto the already-loaded level. False if any joint or
 * link is refused.
 */
[[nodiscard]] inline bool build(bridge::BridgeSystem& system, const Design& design)
{
    for(int i = 0; i < design.joint_count; ++i)
    {
        if(!system.add_node(design.joints[i].x, design.joints[i].y, false))
        {
            return false;
        }
    }

    for(int i = 0; i < design.link_count; ++i)
    {
        const Link& link = design.links[i];

        if(!system.add_strut(link.a, link.b, link.role))
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Load a level with its scheduled engine and weld the par design.
 */
[[nodiscard]] inline bool build_reference(bridge::BridgeSystem& system, const int level_index)
{
    if(level_index < 0 || level_index >= level::COUNT)
    {
        return false;
    }

    system.load_level(level_index);
    system.set_engine(system.level_def().scheduled_engine);
    return build(system, reference(level_index));
}

[[nodiscard]] inline bool run_sim_to_completion(bridge::BridgeSystem& system, int max_frames = 1200)
{
    if(system.begin_test() != bridge::SimStatus::running)
    {
        return false;
    }

    int frames = 0;

    while(system.status() == bridge::SimStatus::running && frames < max_frames)
    {
        system.update();
        ++frames;
    }

    return system.status() == bridge::SimStatus::success;
}

[[nodiscard]] inline bool run_level_playthrough(bridge::BridgeSystem& system, const int level_index,
                                                 int max_frames = 1200)
{
    return build_reference(system, level_index) && run_sim_to_completion(system, max_frames);
}

} // namespace level_solutions

#endif
