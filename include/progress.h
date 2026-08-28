#ifndef MORRIS_BRIDGE_PROGRESS_H
#define MORRIS_BRIDGE_PROGRESS_H

/**
 * @file
 * Level and engine unlock progression for a play session.
 */

#include "engine.h"

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

    void reset();

private:
    void _unlock_engine(engine::Id id);

    bool _try_unlock(engine::Id id, engine::Id& out_unlocked);

    int _max_unlocked_level = 0;
    unsigned _cleared_levels = 0;
    unsigned _unlocked_engines = 0;
};

} // namespace progress

#endif
