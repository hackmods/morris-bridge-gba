#include "progress.h"

#include "bridge_system.h"

namespace progress
{

Progress::Progress()
{
    reset();
}

bool Progress::is_level_unlocked(const int level_index) const
{
    return level_index >= 0 && level_index <= _max_unlocked_level;
}

bool Progress::is_engine_unlocked(const engine::Id id) const
{
    const unsigned bit = 1u << unsigned(id);
    return (_unlocked_engines & bit) != 0;
}

bool Progress::is_level_cleared(const int level_index) const
{
    if(level_index < 0 || level_index >= bridge::LEVEL_COUNT)
    {
        return false;
    }

    return (_cleared_levels & (1u << unsigned(level_index))) != 0;
}

bool Progress::on_level_cleared(const int level_index, engine::Id& out_unlocked)
{
    out_unlocked = engine::Id::count;

    if(level_index < 0 || level_index >= bridge::LEVEL_COUNT)
    {
        return false;
    }

    const unsigned level_bit = 1u << unsigned(level_index);
    const bool first_clear = (_cleared_levels & level_bit) == 0;
    _cleared_levels |= level_bit;

    if(level_index < _max_unlocked_level)
    {
        // already had access to later tracks
    }
    else if(level_index + 1 < bridge::LEVEL_COUNT)
    {
        _max_unlocked_level = level_index + 1;
    }

    engine::Id reward = engine::Id::count;

    switch(level_index)
    {
        case 0:
            reward = engine::Id::percy;
            break;

        case 1:
            reward = engine::Id::edward;
            break;

        case 2:
            reward = engine::Id::james;
            break;

        default:
            break;
    }

    bool unlocked_new = false;

    if(reward != engine::Id::count && ! is_engine_unlocked(reward))
    {
        _unlock_engine(reward);
        out_unlocked = reward;
        unlocked_new = true;
    }

    const unsigned all_cleared = (1u << unsigned(bridge::LEVEL_COUNT)) - 1u;

    if(_cleared_levels == all_cleared && ! is_engine_unlocked(engine::Id::gordon))
    {
        _unlock_engine(engine::Id::gordon);
        out_unlocked = engine::Id::gordon;
        unlocked_new = true;
    }

    (void)first_clear;
    return unlocked_new;
}

void Progress::reset()
{
    _max_unlocked_level = 0;
    _cleared_levels = 0;
    _unlocked_engines = 0;
    _unlock_engine(engine::Id::thomas);
}

void Progress::_unlock_engine(const engine::Id id)
{
    _unlocked_engines |= 1u << unsigned(id);
}

} // namespace progress
