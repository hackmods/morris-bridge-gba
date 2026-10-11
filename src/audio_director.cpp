#include "audio_director.h"

#include "bn_music.h"
#include "bn_music_items.h"

namespace audio
{

namespace
{

constexpr int YARD_ENGINE_MOTIF_BASE = 4;

} // namespace

void Director::ensure_yard_playing()
{
    if(_current != MusicCue::yard)
    {
        _current = MusicCue::yard;
        bn::music_items::yard.play(_normal_volume, true);
    }
}

void Director::play_yard_menu()
{
    ensure_yard_playing();
    bn::music::set_position(0);
}

void Director::play_yard_engine_motif(const engine::Id id)
{
    ensure_yard_playing();
    // The yard tune has five motifs; Henry shares the other big green-flag
    // engine's slot rather than running off the end of the pattern list.
    const engine::Id motif = id == engine::Id::henry ? engine::Id::edward : id;
    bn::music::set_position(YARD_ENGINE_MOTIF_BASE + int(motif) * 2);
}

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
