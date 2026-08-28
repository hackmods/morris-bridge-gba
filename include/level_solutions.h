#ifndef MORRIS_BRIDGE_LEVEL_SOLUTIONS_H
#define MORRIS_BRIDGE_LEVEL_SOLUTIONS_H

/**
 * @file
 * Reference bridge layouts used by host playthrough tests.
 *
 * Knapford Creek: two mid-deck nodes split the 176 px span into three
 * ~44 px legs — within SAFE_LENGTH for Thomas (weight 4).
 */

#include "bridge_system.h"
#include "engine.h"
#include "game_constants.h"

namespace level_solutions
{

inline bool build_knapford_reference(bridge::BridgeSystem& system)
{
    system.load_level(0);
    system.set_engine(engine::Id::thomas);

    const bn::optional<int> mid_a = system.add_node(-44, game::TRACK_Y, false);
    const bn::optional<int> mid_b = system.add_node(44, game::TRACK_Y, false);

    if(!mid_a || !mid_b)
    {
        return false;
    }

    if(!system.add_strut(bridge::START_NODE, mid_a.value()))
    {
        return false;
    }

    if(!system.add_strut(mid_a.value(), mid_b.value()))
    {
        return false;
    }

    if(!system.add_strut(mid_b.value(), bridge::END_NODE))
    {
        return false;
    }

    return true;
}

[[nodiscard]] inline bool run_knapford_playthrough(bridge::BridgeSystem& system, int max_frames = 600)
{
    if(!build_knapford_reference(system))
    {
        return false;
    }

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

} // namespace level_solutions

#endif
