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

    for(const bridge::Strut& strut : system.struts())
    {
        if(strut.role == bridge::LinkRole::brace || strut.role == bridge::LinkRole::foundation)
        {
            ++score.brace_links;
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

    return score;
}

} // namespace session
