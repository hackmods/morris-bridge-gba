#include "audio_director.h"

#include "bn_music.h"
#include "bn_music_items.h"

namespace audio
{

void Director::play(const MusicCue cue)
{
    if(cue == _current)
    {
        return;
    }

    _current = cue;

    switch(cue)
    {
        case MusicCue::none:
            bn::music::stop();
            break;

        case MusicCue::island_line:
            bn::music_items::island_line.play(_normal_volume, true);
            break;

        case MusicCue::yard:
            bn::music_items::yard.play(_normal_volume, true);
            break;

        case MusicCue::workshop:
            bn::music_items::workshop.play(_normal_volume, true);
            break;

        case MusicCue::full_steam:
            bn::music_items::full_steam.play(_normal_volume, true);
            break;
    }
}

void Director::duck_for_pause()
{
    if(bn::music::playing())
    {
        bn::music::set_volume(_pause_volume);
    }
}

void Director::restore_from_pause()
{
    if(bn::music::playing())
    {
        bn::music::set_volume(_normal_volume);
    }
}

} // namespace audio
