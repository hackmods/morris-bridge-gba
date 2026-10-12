#ifndef MORRIS_BRIDGE_RUN_SCORE_H
#define MORRIS_BRIDGE_RUN_SCORE_H

/**
 * @file
 * End-of-level efficiency scoring (joints + links vs par, on the scheduled engine).
 */

#include "bridge_system.h"
#include "level.h"

namespace session
{

struct RunScore
{
    int player_nodes = 0;
    int track_links = 0;
    int brace_links = 0;
    int cable_links = 0;
    int total_cost = 0;
    uint8_t stars = 0;
    bool under_weight = false;  ///< Crossed with a lighter train than the timetable's.
};

[[nodiscard]] RunScore compute_run_score(const bridge::BridgeSystem& system, const level::Def& def);

/**
 * @brief The most a bridge on this level may cost: twice par.
 *
 * Par earns the stars; the purse is the hard limit that stops a level being
 * brute-forced with a wall of timber, with room left to over-build and learn.
 */
[[nodiscard]] int purse(const level::Def& def);

/// What is left of the purse after the bridge as it stands.
[[nodiscard]] int purse_left(const bridge::BridgeSystem& system, const level::Def& def);

} // namespace session

#endif
