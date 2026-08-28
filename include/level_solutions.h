#ifndef MORRIS_BRIDGE_LEVEL_SOLUTIONS_H
#define MORRIS_BRIDGE_LEVEL_SOLUTIONS_H

/**
 * @file
 * Reference bridge layouts used by host playthrough tests.
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

    if(!system.add_strut(bridge::START_NODE, mid_a.value(), bridge::LinkRole::track))
    {
        return false;
    }

    if(!system.add_strut(mid_a.value(), mid_b.value(), bridge::LinkRole::track))
    {
        return false;
    }

    if(!system.add_strut(mid_b.value(), bridge::END_NODE, bridge::LinkRole::track))
    {
        return false;
    }

    return true;
}

inline bool build_gordon_hill_reference(bridge::BridgeSystem& system)
{
    system.load_level(1);
    system.set_engine(engine::Id::edward);

    const bn::fixed deck = system.level_def().deck_y;
    const bn::optional<int> n1 = system.add_node(-52, deck, false);
    const bn::optional<int> n2 = system.add_node(-8, deck, false);
    const bn::optional<int> n3 = system.add_node(36, deck, false);
    const bn::optional<int> n4 = system.add_node(80, deck, false);

    if(!n1 || !n2 || !n3 || !n4)
    {
        return false;
    }

    const int mids[] = { n1.value(), n2.value(), n3.value(), n4.value() };
    int prev = bridge::START_NODE;

    for(int mid : mids)
    {
        if(!system.add_strut(prev, mid, bridge::LinkRole::track))
        {
            return false;
        }

        prev = mid;
    }

    if(!system.add_strut(prev, bridge::END_NODE, bridge::LinkRole::track))
    {
        return false;
    }

    if(!system.add_strut(bridge::START_NODE, 3, bridge::LinkRole::brace))
    {
        return false;
    }

    if(!system.add_strut(bridge::END_NODE, 4, bridge::LinkRole::brace))
    {
        return false;
    }

    return true;
}

[[nodiscard]] inline bool run_sim_to_completion(bridge::BridgeSystem& system, int max_frames = 600)
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

[[nodiscard]] inline bool run_knapford_playthrough(bridge::BridgeSystem& system, int max_frames = 600)
{
    return build_knapford_reference(system) && run_sim_to_completion(system, max_frames);
}

[[nodiscard]] inline bool run_gordon_hill_playthrough(bridge::BridgeSystem& system, int max_frames = 800)
{
    return build_gordon_hill_reference(system) && run_sim_to_completion(system, max_frames);
}

} // namespace level_solutions

#endif
