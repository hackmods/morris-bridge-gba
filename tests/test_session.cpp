/**
 * @file tests/test_session.cpp
 *
 * Host tests for session scoring helpers.
 */

#include <cassert>
#include <iostream>

#include "bridge_system.h"
#include "level.h"
#include "level_solutions.h"
#include "run_score.h"

int main()
{
    std::cout << "Thomas's Bridge session tests\n";

    {
        bridge::BridgeSystem system;
        system.load_level(0);
        const session::RunScore empty = session::compute_run_score(system, system.level_def());
        assert(empty.total_cost == 0);
        assert(empty.player_nodes == 0);
        assert(empty.track_links == 0);
        std::cout << "  ok empty bridge has zero build cost\n";
    }

    {
        bridge::BridgeSystem system;
        assert(level_solutions::build_reference(system, 1));
        const session::RunScore score =
            session::compute_run_score(system, system.level_def());
        assert(score.player_nodes == 3);
        assert(score.track_links == 4);
        assert(score.cable_links == 3);
        assert(score.brace_links == 0);
        assert(score.total_cost == 10);
        std::cout << "  ok cables cost 1 each on Gordon's Hill\n";
    }

    {
        for(int level_index = 0; level_index < level::COUNT; ++level_index)
        {
            bridge::BridgeSystem system;
            assert(level_solutions::run_level_playthrough(system, level_index));
            const session::RunScore score =
                session::compute_run_score(system, system.level_def());
            assert(score.stars == 3);
            assert(!score.under_weight);
            assert(score.total_cost == system.level_def().par_cost);
        }

        std::cout << "  ok every shipped track is won at par for 3 stars on its scheduled engine\n";
    }

    {
        // Vicarstown is timetabled for Edward; Percy is lighter than that.
        bridge::BridgeSystem system;
        assert(level_solutions::build_reference(system, 2));
        system.set_engine(engine::Id::percy);
        assert(level_solutions::run_sim_to_completion(system));
        const session::RunScore light =
            session::compute_run_score(system, system.level_def());
        assert(light.under_weight);
        assert(light.stars == 1);

        system.end_test();
        system.set_engine(engine::Id::james);
        assert(level_solutions::run_sim_to_completion(system));
        const session::RunScore heavy =
            session::compute_run_score(system, system.level_def());
        assert(!heavy.under_weight);
        assert(heavy.stars == 3);
        std::cout << "  ok a lighter engine than the timetable's clears the track for one star\n";
    }

    {
        // Over-building still wins, for fewer stars.
        bridge::BridgeSystem system;
        assert(level_solutions::build_reference(system, 0));
        const bn::optional<int> spare = system.add_node(-24, 12 + 14, false);
        assert(spare.has_value());
        assert(system.add_strut(bridge::START_NODE, spare.value()));
        assert(system.add_strut(spare.value(), 2));
        assert(level_solutions::run_sim_to_completion(system));
        const session::RunScore score =
            session::compute_run_score(system, system.level_def());
        assert(score.total_cost == system.level_def().par_cost + 3);
        assert(score.stars == 1);
        std::cout << "  ok cost over par loses stars\n";
    }

    {
        // A beam that drops into the river mid-run was still paid for.
        bridge::BridgeSystem system;
        assert(level_solutions::build_reference(system, 0));
        const bn::optional<int> a = system.add_node(-24, 0, false);
        const bn::optional<int> b = system.add_node(24, 0, false);
        assert(a && b);
        assert(system.add_strut(a.value(), b.value()));
        const session::RunScore before =
            session::compute_run_score(system, system.level_def());
        assert(level_solutions::run_sim_to_completion(system));
        const session::RunScore after =
            session::compute_run_score(system, system.level_def());
        assert(after.total_cost == before.total_cost);
        std::cout << "  ok score counts the bridge as built\n";
    }

    {
        // The purse is twice par, and every par design fits.
        for(int level_index = 0; level_index < level::COUNT; ++level_index)
        {
            bridge::BridgeSystem system;
            assert(level_solutions::build_reference(system, level_index));
            const level::Def& def = system.level_def();
            assert(session::purse(def) == def.par_cost * 2);
            assert(session::purse_left(system, def) == session::purse(def) - def.par_cost);
        }

        std::cout << "  ok the purse leaves room over par on every track\n";
    }

    std::cout << "All session tests passed.\n";
    return 0;
}
