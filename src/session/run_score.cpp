#include "run_score.h"

namespace session
{
namespace
{

[[nodiscard]] uint8_t stars_for_cost(const int cost, const int par)
{
    if(par <= 0)
    {
        return 1;
    }

    if(cost <= par)
    {
        return 3;
    }

    if(cost <= par + (par / 4 + 1))
    {
        return 2;
    }

    return 1;
}

} // namespace

RunScore compute_run_score(const bridge::BridgeSystem& system, const level::Def& def)
{
    RunScore score;
    score.player_nodes = system.player_node_count();

    for(const bridge::Strut& strut : system.built_struts())
    {
        if(strut.role == bridge::LinkRole::brace)
        {
            ++score.brace_links;
        }
        else if(strut.role == bridge::LinkRole::cable)
        {
            ++score.cable_links;
        }
        else
        {
            ++score.track_links;
        }

        score.total_cost += bridge::link_cost(strut.material);
    }

    score.total_cost += score.player_nodes * bridge::NODE_COST;

    if(def.par_cost > 0)
    {
        score.stars = stars_for_cost(score.total_cost, def.par_cost);
    }
    else
    {
        score.stars = 1;
    }

    // Sir Topham Hatt's timetable names the engine. A lighter one still clears
    // the track, but it is not the crossing the level asked for.
    if(system.train_total_weight() < engine::total_weight(def.scheduled_engine))
    {
        score.under_weight = true;
        score.stars = 1;
    }

    return score;
}

int purse(const level::Def& def)
{
    return def.par_cost * 2;
}

int purse_left(const bridge::BridgeSystem& system, const level::Def& def)
{
    return purse(def) - compute_run_score(system, def).total_cost;
}

} // namespace session
