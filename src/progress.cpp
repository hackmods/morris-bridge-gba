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

void Progress::apply_save(const int max_unlocked_level, const unsigned cleared_levels,
                          const unsigned unlocked_engines)
{
    _max_unlocked_level = max_unlocked_level;
    _cleared_levels = cleared_levels;
    _unlocked_engines = unlocked_engines | (1u << unsigned(engine::default_engine()));
}

void Progress::record_level_result(const int level_index, const uint8_t stars,
                                     const uint16_t cost)
{
    if(level_index < 0 || level_index >= level::COUNT || stars == 0)
    {
        return;
    }

    const uint8_t prev_stars = _best_stars[level_index];
    const uint16_t prev_cost = _best_cost[level_index];
    const bool better_stars = stars > prev_stars;
    const bool same_stars_better_cost =
        stars == prev_stars && stars > 0 && (prev_cost == 0 || cost < prev_cost);

    if(prev_stars == 0 || better_stars || same_stars_better_cost)
    {
        _best_stars[level_index] = stars;
        _best_cost[level_index] = cost;
    }
}

uint8_t Progress::best_stars(const int level_index) const
{
    if(level_index < 0 || level_index >= level::COUNT)
    {
        return 0;
    }

    return _best_stars[level_index];
}

uint16_t Progress::best_cost(const int level_index) const
{
    if(level_index < 0 || level_index >= level::COUNT)
    {
        return 0;
    }

    return _best_cost[level_index];
}

uint8_t Progress::packed_best_stars() const
{
    uint8_t packed = 0;

    for(int i = 0; i < level::COUNT; ++i)
    {
        packed |= uint8_t((_best_stars[i] & 3) << (i * 2));
    }

    return packed;
}

void Progress::apply_save_v2(const int max_unlocked_level, const unsigned cleared_levels,
                             const unsigned unlocked_engines, const uint8_t packed_stars,
                             const uint16_t best_costs[level::COUNT])
{
    apply_save(max_unlocked_level, cleared_levels, unlocked_engines);

    for(int i = 0; i < level::COUNT; ++i)
    {
        _best_stars[i] = uint8_t((packed_stars >> (i * 2)) & 3);
        _best_cost[i] = best_costs[i];
    }
}

void Progress::apply_save_v3(const int max_unlocked_level, const unsigned cleared_levels,
                             const unsigned unlocked_engines, const uint8_t packed_stars,
                             const uint16_t best_costs[level::COUNT], const uint8_t intro_seen_mask)
{
    apply_save_v2(max_unlocked_level, cleared_levels, unlocked_engines, packed_stars, best_costs);
    _intro_seen = intro_seen_mask;
}

bool Progress::is_intro_seen(const int level_index) const
{
    if(level_index < 0 || level_index >= level::COUNT)
    {
        return true;
    }

    return (_intro_seen & (1u << unsigned(level_index))) != 0;
}

void Progress::mark_intro_seen(const int level_index)
{
    if(level_index < 0 || level_index >= level::COUNT)
    {
        return;
    }

    _intro_seen |= 1u << unsigned(level_index);
}

void Progress::reset()
{
    _max_unlocked_level = 0;
    _cleared_levels = 0;
    _unlocked_engines = 0;
    _intro_seen = 0;

    for(int i = 0; i < level::COUNT; ++i)
    {
        _best_stars[i] = 0;
        _best_cost[i] = 0;
    }

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
