#ifndef MORRIS_BRIDGE_SCREEN_TYPES_H
#define MORRIS_BRIDGE_SCREEN_TYPES_H

/**
 * @file
 * Screen flow identifiers shared by Game and future ScreenManager.
 */

namespace screen
{

enum class Id
{
    title,
    level_select,
    engine_select,
    level_intro,
    playing,
    pause,
    level_complete,
    level_failed,
    all_complete,
};

enum class PlayMode
{
    build,
    test,
};

enum class PauseOption
{
    resume = 0,
    restart,
    levels,
    title,
    count,
};

} // namespace screen

#endif
