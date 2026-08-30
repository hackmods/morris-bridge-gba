#ifndef MORRIS_BRIDGE_LEVEL_SOLUTIONS_H
#define MORRIS_BRIDGE_LEVEL_SOLUTIONS_H

/**
 * @file
 * Reference bridge layouts used by host playthrough tests.
 */

#include "bridge_system.h"
#include "engine.h"
#include "game_constants.h"
#include "level.h"

namespace level_solutions
{
namespace
{

inline bool weld_deck(bridge::BridgeSystem& system, const int* xs, int count)
{
    const bn::fixed deck = system.level_def().deck_y;
    int prev = bridge::START_NODE;

    for(int i = 0; i < count; ++i)
    {
        const bn::optional<int> mid = system.add_node(xs[i], deck, false);

        if(!mid)
        {
            return false;
        }

        if(!system.add_strut(prev, mid.value(), bridge::LinkRole::track).has_value())
        {
            return false;
        }

        prev = mid.value();
    }

    return system.add_strut(prev, bridge::END_NODE, bridge::LinkRole::track).has_value();
}

inline bool weld_end_braces(bridge::BridgeSystem& system, bool left, bool right)
{
    if(left && !system.add_strut(bridge::START_NODE, 3, bridge::LinkRole::brace).has_value())
    {
        return false;
    }

    if(right && !system.add_strut(bridge::END_NODE, 4, bridge::LinkRole::brace).has_value())
    {
        return false;
    }

    return true;
}

} // namespace

inline bool build_knapford_reference(bridge::BridgeSystem& system)
{
    system.load_level(0);
    system.set_engine(engine::Id::thomas);
    const int xs[] = { -44, 44 };
    return weld_deck(system, xs, 2);
}

inline bool build_gordon_hill_reference(bridge::BridgeSystem& system)
{
    system.load_level(1);
    system.set_engine(engine::Id::edward);
    const int xs[] = { -52, -8, 36, 80 };
    return weld_deck(system, xs, 4) && weld_end_braces(system, true, true);
}

inline bool build_vicarstown_reference(bridge::BridgeSystem& system)
{
    system.load_level(2);
    system.set_engine(engine::Id::james);
    const int xs[] = { -64, -24, 16, 56, 96 };
    return weld_deck(system, xs, 5) && weld_end_braces(system, true, false);
}

inline bool build_tidmouth_reference(bridge::BridgeSystem& system)
{
    system.load_level(3);
    system.set_engine(engine::Id::percy);
    const int xs[] = { -24, 24 };
    return weld_deck(system, xs, 2);
}

inline bool build_wellsworth_reference(bridge::BridgeSystem& system)
{
    system.load_level(4);
    system.set_engine(engine::Id::thomas);
    const int xs[] = { -50, 0, 50 };
    return weld_deck(system, xs, 3) && weld_end_braces(system, true, true);
}

inline bool build_crovan_reference(bridge::BridgeSystem& system)
{
    system.load_level(5);
    system.set_engine(engine::Id::edward);
    const int xs[] = { -52, -8, 36, 80 };
    return weld_deck(system, xs, 4) && weld_end_braces(system, true, true);
}

inline bool build_callan_reference(bridge::BridgeSystem& system)
{
    system.load_level(6);
    system.set_engine(engine::Id::james);
    const int xs[] = { -72, -32, 8, 48, 88 };
    return weld_deck(system, xs, 5) && weld_end_braces(system, true, true);
}

inline bool build_summit_reference(bridge::BridgeSystem& system)
{
    system.load_level(7);
    system.set_engine(engine::Id::james);
    const int xs[] = { -80, -48, -16, 16, 48, 80 };
    return weld_deck(system, xs, 6) && weld_end_braces(system, true, true);
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

[[nodiscard]] inline bool run_vicarstown_playthrough(bridge::BridgeSystem& system, int max_frames = 900)
{
    return build_vicarstown_reference(system) && run_sim_to_completion(system, max_frames);
}

[[nodiscard]] inline bool run_tidmouth_playthrough(bridge::BridgeSystem& system, int max_frames = 700)
{
    return build_tidmouth_reference(system) && run_sim_to_completion(system, max_frames);
}

[[nodiscard]] inline bool run_wellsworth_playthrough(bridge::BridgeSystem& system, int max_frames = 800)
{
    return build_wellsworth_reference(system) && run_sim_to_completion(system, max_frames);
}

[[nodiscard]] inline bool run_crovan_playthrough(bridge::BridgeSystem& system, int max_frames = 900)
{
    return build_crovan_reference(system) && run_sim_to_completion(system, max_frames);
}

[[nodiscard]] inline bool run_callan_playthrough(bridge::BridgeSystem& system, int max_frames = 1000)
{
    return build_callan_reference(system) && run_sim_to_completion(system, max_frames);
}

[[nodiscard]] inline bool run_summit_playthrough(bridge::BridgeSystem& system, int max_frames = 1100)
{
    return build_summit_reference(system) && run_sim_to_completion(system, max_frames);
}

inline bool build_reference(bridge::BridgeSystem& system, int level_index)
{
    switch(level_index)
    {
        case 0:
            return build_knapford_reference(system);

        case 1:
            return build_gordon_hill_reference(system);

        case 2:
            return build_vicarstown_reference(system);

        case 3:
            return build_tidmouth_reference(system);

        case 4:
            return build_wellsworth_reference(system);

        case 5:
            return build_crovan_reference(system);

        case 6:
            return build_callan_reference(system);

        case 7:
            return build_summit_reference(system);

        default:
            return false;
    }
}

[[nodiscard]] inline bool run_level_playthrough(bridge::BridgeSystem& system, int level_index,
                                                 int max_frames = 1100)
{
    return build_reference(system, level_index) && run_sim_to_completion(system, max_frames);
}

} // namespace level_solutions

#endif
