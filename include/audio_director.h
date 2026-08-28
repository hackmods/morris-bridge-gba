#ifndef MORRIS_BRIDGE_AUDIO_DIRECTOR_H
#define MORRIS_BRIDGE_AUDIO_DIRECTOR_H

/**
 * @file
 * Single owner for background music cues and pause ducking.
 */

#include "bn_fixed.h"

namespace audio
{

inline constexpr bn::fixed NORMAL_VOLUME = bn::fixed(1) / 2;
inline constexpr bn::fixed PAUSE_VOLUME = bn::fixed(1) / 5;

enum class MusicCue
{
    none,
    island_line,
    yard,
    workshop,
    full_steam,
};

class Director
{
public:
    void play(MusicCue cue);
    void duck_for_pause();
    void restore_from_pause();

    [[nodiscard]] MusicCue current() const
    {
        return _current;
    }

private:
    MusicCue _current = MusicCue::none;
    bn::fixed _normal_volume = NORMAL_VOLUME;
    bn::fixed _pause_volume = PAUSE_VOLUME;
};

} // namespace audio

#endif
