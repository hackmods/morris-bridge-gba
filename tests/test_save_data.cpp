/**
 * @file tests/test_save_data.cpp
 */

#include <cassert>
#include <fstream>
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
    for(int level_index = 0; level_index < level::COUNT; ++level_index)
    {
        (void)progress.on_level_cleared(level_index, unlocked);
    }
    assert(progress.is_engine_unlocked(engine::Id::gordon));
    assert(save_data::encode(progress, buffer));
    loaded.reset();
    assert(save_data::decode(loaded, buffer));
    assert(loaded.is_engine_unlocked(engine::Id::gordon));
    assert(loaded.is_level_unlocked(level::COUNT - 1));
    std::cout << "  ok round-trip full unlock state\n";

    buffer[8] ^= 0xFF;
    assert(! save_data::decode(loaded, buffer));
    std::cout << "  ok checksum rejects tampered save\n";

    progress.reset();
    progress.record_level_result(0, 3, 5);
    progress.record_level_result(1, 2, 11);
    progress.record_level_result(2, 1, 15);
    progress.record_level_result(7, 3, 16);
    assert(save_data::encode(progress, buffer));
    loaded.reset();
    assert(save_data::decode(loaded, buffer));
    assert(loaded.best_stars(0) == 3);
    assert(loaded.best_cost(0) == 5);
    assert(loaded.best_stars(1) == 2);
    assert(loaded.best_cost(1) == 11);
    assert(loaded.best_stars(2) == 1);
    assert(loaded.best_cost(2) == 15);
    assert(loaded.best_stars(7) == 3);
    assert(loaded.best_cost(7) == 16);
    std::cout << "  ok v4 round-trip best stars and cost\n";

    progress.record_level_result(0, 2, 4);
    assert(progress.best_stars(0) == 3);
    assert(progress.best_cost(0) == 5);
    progress.record_level_result(0, 3, 6);
    assert(progress.best_stars(0) == 3);
    assert(progress.best_cost(0) == 5);
    progress.record_level_result(0, 3, 4);
    assert(progress.best_stars(0) == 3);
    assert(progress.best_cost(0) == 4);
    std::cout << "  ok record_level_result keeps best stars and lowest cost\n";

    std::uint8_t v1[save_data::SIZE] = {};
    v1[0] = std::uint8_t(save_data::MAGIC & 0xFF);
    v1[1] = std::uint8_t((save_data::MAGIC >> 8) & 0xFF);
    v1[2] = std::uint8_t((save_data::MAGIC >> 16) & 0xFF);
    v1[3] = std::uint8_t((save_data::MAGIC >> 24) & 0xFF);
    v1[4] = save_data::VERSION_V1;
    v1[5] = 2;
    v1[6] = 0b111;
    v1[7] = 0b1111111;
    std::uint8_t sum = 0;

    for(int i = 0; i < 8; ++i)
    {
        sum = std::uint8_t(sum + v1[i]);
    }

    v1[8] = sum;
    loaded.reset();
    assert(save_data::decode(loaded, v1));
    assert(loaded.max_unlocked_level() == 2);
    assert(loaded.best_stars(0) == 0);
    assert(loaded.best_cost(0) == 0);
    std::cout << "  ok v1 save migrates with zero stars\n";

    progress.reset();
    progress.mark_intro_seen(0);
    progress.mark_intro_seen(2);
    assert(save_data::encode(progress, buffer));
    loaded.reset();
    assert(save_data::decode(loaded, buffer));
    assert(loaded.is_intro_seen(0));
    assert(! loaded.is_intro_seen(1));
    assert(loaded.is_intro_seen(2));
    std::cout << "  ok v3 round-trip intro seen flags\n";

    {
        std::uint8_t v3[save_data::SIZE] = {};
        v3[0] = std::uint8_t(save_data::MAGIC & 0xFF);
        v3[1] = std::uint8_t((save_data::MAGIC >> 8) & 0xFF);
        v3[2] = std::uint8_t((save_data::MAGIC >> 16) & 0xFF);
        v3[3] = std::uint8_t((save_data::MAGIC >> 24) & 0xFF);
        v3[4] = save_data::VERSION_V3;
        v3[5] = 2;
        v3[6] = 0b111;
        v3[7] = 0b00111;
        v3[8] = 0b111011;
        v3[9] = 5;
        v3[10] = 0;
        v3[11] = 11;
        v3[12] = 0;
        v3[13] = 12;
        v3[14] = 0;
        v3[15] = 0b111;
        std::uint8_t v3_sum = 0;

        for(int i = 0; i < 19; ++i)
        {
            v3_sum = std::uint8_t(v3_sum + v3[i]);
        }

        v3[19] = v3_sum;
        loaded.reset();
        assert(save_data::decode(loaded, v3));
        assert(loaded.max_unlocked_level() == 2);
        assert(loaded.best_stars(0) == 3);
        assert(loaded.best_cost(0) == 5);
        assert(loaded.best_stars(7) == 0);
        assert(loaded.is_intro_seen(2));
        std::cout << "  ok v3 save migrates into eight-track progress\n";
    }

    {
        std::ifstream file("../web/public/saves/gordon_hill_walkthrough.sav", std::ios::binary);
        assert(file.good());
        std::uint8_t sav[save_data::SIZE] = {};
        file.read(reinterpret_cast<char*>(sav), save_data::SIZE);
        assert(file.gcount() == save_data::SIZE);
        loaded.reset();
        assert(save_data::decode(loaded, sav));
        assert(loaded.is_level_unlocked(1));
        assert(loaded.is_intro_seen(0));
        assert(loaded.is_intro_seen(1));
        assert(loaded.is_engine_unlocked(engine::Id::thomas));
        assert(loaded.is_engine_unlocked(engine::Id::percy));
        std::cout << "  ok gordon_hill_walkthrough.sav decodes for playtest\n";
    }

    std::cout << "All save_data tests passed.\n";
    return 0;
}
