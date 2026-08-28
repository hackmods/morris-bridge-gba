/**
 * @file tests/test_save_data.cpp
 */

#include <cassert>
#include <iostream>

#include "progress.h"
#include "save_data.h"

int main()
{
    std::cout << "Morris Bridge save_data tests\n";

    progress::Progress progress;
    std::uint8_t buffer[save_data::SIZE] = {};

    assert(save_data::encode(progress, buffer));
    progress::Progress loaded;
    assert(save_data::decode(loaded, buffer));
    assert(loaded.is_engine_unlocked(engine::Id::thomas));
    assert(! loaded.is_engine_unlocked(engine::Id::percy));
    std::cout << "  ok round-trip fresh progress\n";

    engine::Id unlocked = engine::none;
    (void)progress.on_level_cleared(0, unlocked);
    (void)progress.on_level_cleared(1, unlocked);
    (void)progress.on_level_cleared(2, unlocked);
    assert(progress.is_engine_unlocked(engine::Id::gordon));
    assert(save_data::encode(progress, buffer));
    loaded.reset();
    assert(save_data::decode(loaded, buffer));
    assert(loaded.is_engine_unlocked(engine::Id::gordon));
    assert(loaded.is_level_unlocked(2));
    std::cout << "  ok round-trip full unlock state\n";

    buffer[8] ^= 0xFF;
    assert(! save_data::decode(loaded, buffer));
    std::cout << "  ok checksum rejects tampered save\n";

    std::cout << "All save_data tests passed.\n";
    return 0;
}
