#ifndef MORRIS_BRIDGE_PROGRESS_H
#define MORRIS_BRIDGE_PROGRESS_H

/**
 * @file
 * Level and engine unlock progression for a play session.
 */

#include "engine.h"
#include "level.h"

#include <cstdint>

namespace progress
{

class Progress
{
public:
    Progress();

    [[nodiscard]] int max_unlocked_level() const
    {
        return _max_unlocked_level;
    }

    [[nodiscard]] bool is_level_unlocked(int level_index) const;

    [[nodiscard]] bool is_engine_unlocked(engine::Id id) const;

    [[nodiscard]] bool is_level_cleared(int level_index) const;

    /**
     * @brief Record a level win and apply unlocks.
     * @return Newly unlocked engine, or empty if none this call.
     */
    [[nodiscard]] bool on_level_cleared(int level_index, engine::Id& out_unlocked);

    /**
     * @brief Keep the best star rating and lowest cost for a cleared level.
     */
    void record_level_result(int level_index, uint8_t stars, uint16_t cost);

    [[nodiscard]] uint8_t best_stars(int level_index) const;

    [[nodiscard]] uint16_t best_cost(int level_index) const;

    void reset();

    void apply_save(int max_unlocked_level, unsigned cleared_levels, unsigned unlocked_engines);

    void apply_save_v2(int max_unlocked_level, unsigned cleared_levels, unsigned unlocked_engines,
                       uint8_t packed_stars, const uint16_t best_costs[level::COUNT]);

    [[nodiscard]] unsigned cleared_levels_mask() const
    {
        return _cleared_levels;
    }

    [[nodiscard]] unsigned unlocked_engines_mask() const
    {
        return _unlocked_engines;
    }

    [[nodiscard]] uint8_t packed_best_stars() const;

    [[nodiscard]] const uint16_t* best_costs() const
    {
        return _best_cost;
    }

private:
    void _unlock_engine(engine::Id id);

    bool _try_unlock(engine::Id id, engine::Id& out_unlocked);

    int _max_unlocked_level = 0;
    unsigned _cleared_levels = 0;
    unsigned _unlocked_engines = 0;
    uint8_t _best_stars[level::COUNT] = {};
    uint16_t _best_cost[level::COUNT] = {};
};

} // namespace progress

#endif
