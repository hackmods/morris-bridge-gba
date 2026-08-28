/**
 * @file tests/test_progress.cpp
 *
 * Host tests for unlock progression (no GBA toolchain required).
 */

#include <cassert>
#include <iostream>

#include "bridge_system.h"
#include "engine.h"
#include "progress.h"

int main()
{
    std::cout << "Morris Bridge progress tests\n";

    progress::Progress progress;
    assert(progress.is_engine_unlocked(engine::Id::thomas));
    assert(! progress.is_engine_unlocked(engine::Id::percy));
    assert(progress.is_level_unlocked(0));
    assert(! progress.is_level_unlocked(1));
    std::cout << "  ok fresh session starts with Thomas on Knapford only\n";

    engine::Id unlocked = engine::none;
    assert(progress.on_level_cleared(0, unlocked));
    assert(unlocked == engine::Id::percy);
    assert(progress.is_engine_unlocked(engine::Id::percy));
    assert(progress.is_level_unlocked(1));
    assert(! progress.is_level_unlocked(2));
    std::cout << "  ok clearing level 0 unlocks Percy and Gordon's Hill\n";

    unlocked = engine::none;
    assert(! progress.on_level_cleared(0, unlocked));
    assert(unlocked == engine::none);
    std::cout << "  ok repeat clear of level 0 does not re-unlock\n";

    unlocked = engine::none;
    assert(progress.on_level_cleared(1, unlocked));
    assert(unlocked == engine::Id::edward);
    assert(progress.is_level_unlocked(2));
    std::cout << "  ok clearing level 1 unlocks Edward and Vicarstown\n";

    unlocked = engine::none;
    assert(progress.on_level_cleared(2, unlocked));
    assert(unlocked == engine::Id::james);
    assert(progress.is_engine_unlocked(engine::Id::gordon));
    std::cout << "  ok clearing level 2 unlocks James and Gordon (all tracks)\n";

    unlocked = engine::none;
    assert(! progress.on_level_cleared(2, unlocked));
    std::cout << "  ok repeat clear of level 2 does not re-unlock\n";

    progress.reset();
    assert(progress.is_engine_unlocked(engine::Id::thomas));
    assert(! progress.is_engine_unlocked(engine::Id::gordon));
    std::cout << "  ok reset restores starter state\n";

    std::cout << "All progress tests passed.\n";
    return 0;
}
