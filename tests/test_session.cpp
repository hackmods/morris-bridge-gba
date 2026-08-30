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
    std::cout << "Morris Bridge session tests\n";

    {
        bridge::BridgeSystem system;
        assert(level_solutions::build_knapford_reference(system));
        const session::RunScore score =
            session::compute_run_score(system, system.level_def());
        assert(score.stars == 3);
        assert(score.total_cost <= system.level_def().par_cost);
        std::cout << "  ok Knapford reference meets par for 3 stars\n";
    }

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
        system.load_level(1);
        const level::Def& def = system.level_def();
        assert(def.par_cost == 11);
        assert(def.bg_theme_id == 1);
        std::cout << "  ok Gordon's Hill par and theme id\n";
    }

    {
        for(int level_index = 0; level_index < level::COUNT; ++level_index)
        {
            bridge::BridgeSystem system;
            assert(level_solutions::build_reference(system, level_index));
            const session::RunScore score =
                session::compute_run_score(system, system.level_def());
            assert(score.stars == 3);
            assert(score.total_cost <= system.level_def().par_cost);
        }

        std::cout << "  ok every shipped track has a 3-star reference\n";
    }

    std::cout << "All session tests passed.\n";
    return 0;
}
