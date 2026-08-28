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
    if(! engine::is_playable(id))
    {
        return false;
    }

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

bool Progress::_try_unlock(const engine::Id id, engine::Id& out_unlocked)
{
    if(! engine::is_playable(id) || is_engine_unlocked(id))
    {
        return false;
    }

    _unlock_engine(id);
    out_unlocked = id;
    return true;
}

bool Progress::on_level_cleared(const int level_index, engine::Id& out_unlocked)
{
    out_unlocked = engine::none;

    if(level_index < 0 || level_index >= bridge::LEVEL_COUNT)
    {
        return false;
    }

    const unsigned level_bit = 1u << unsigned(level_index);
    _cleared_levels |= level_bit;

    if(level_index >= _max_unlocked_level && level_index + 1 < bridge::LEVEL_COUNT)
    {
        _max_unlocked_level = level_index + 1;
    }

    bool unlocked_new = false;

    if(_try_unlock(engine::unlock_for_level_clear(level_index), out_unlocked))
    {
        unlocked_new = true;
    }

    const unsigned all_cleared = (1u << unsigned(bridge::LEVEL_COUNT)) - 1u;

    if(_cleared_levels == all_cleared)
    {
        engine::Id bonus = engine::none;

        if(_try_unlock(engine::all_tracks_bonus_engine(), bonus))
        {
            if(! unlocked_new)
            {
                out_unlocked = bonus;
            }

            unlocked_new = true;
        }
    }

    return unlocked_new;
}

void Progress::reset()
{
    _max_unlocked_level = 0;
    _cleared_levels = 0;
    _unlocked_engines = 0;
    _unlock_engine(engine::default_engine());
}

void Progress::_unlock_engine(const engine::Id id)
{
    if(! engine::is_playable(id))
    {
        return;
    }

    _unlocked_engines |= 1u << unsigned(id);
}

} // namespace progress
