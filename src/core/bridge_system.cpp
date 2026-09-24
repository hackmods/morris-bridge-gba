#include "bridge_system.h"

#include "game_constants.h"
#include "level.h"

namespace bridge
{
namespace
{
int min_id(int a, int b)
{
    return a < b ? a : b;
}

int max_id(int a, int b)
{
    return a > b ? a : b;
}

} // namespace

BridgeSystem::BridgeSystem()
{
    set_engine(engine::default_engine());
    load_level(0);
}

void BridgeSystem::set_engine(const engine::Id id)
{
    _engine = id;
    const engine::Profile& profile = engine::profile(id);
    _train_weight = profile.weight;
    _train_speed = profile.speed;
    _coach_count = profile.coach_count;
}

bn::fixed BridgeSystem::effective_weight_at(const bn::fixed distance_along,
                                            const bn::fixed strut_length) const
{
    if(strut_length <= 0)
    {
        return _train_weight;
    }

    const bn::fixed ratio = distance_along.safe_division(strut_length);
    const bn::fixed coach_load =
        bn::fixed(_coach_count).safe_multiplication(physics::COACH_WEIGHT).safe_multiplication(ratio);
    return _train_weight + coach_load;
}

void BridgeSystem::load_level(int level_index)
{
    if(level_index < 0)
    {
        level_index = 0;
    }

    if(level_index >= LEVEL_COUNT)
    {
        level_index = LEVEL_COUNT - 1;
    }

    _level_index = level_index;
    const level::Def& def = level::get(_level_index);
    _safe_length = def.safe_length > 0 ? def.safe_length : physics::SAFE_LENGTH;
    reset_level();
}

void BridgeSystem::reset_level()
{
    _nodes.clear();
    _struts.clear();
    _saved_struts.clear();
    _path.clear();
    _status = SimStatus::idle;
    _train_t = 0;
    _path_index = 0;
    _accel_frames = 0;
    _fall_vy = 0;
    _falling = false;
    _last_edit = LastEdit::none;
    _last_edit_index = -1;
    _train_state = TrainState::waiting;
    _seed_level();
    _train_position = _nodes[START_NODE].position;
}

void BridgeSystem::_seed_level()
{
    _anchored_node_count = 0;
    const level::Def& def = level::get(_level_index);

    for(int i = 0; i < def.seed_count; ++i)
    {
        const level::Seed& seed = def.seeds[i];
        (void)add_node(seed.x, seed.y, seed.anchored);
    }
}

bn::optional<int> BridgeSystem::add_node(bn::fixed x, bn::fixed y, bool is_anchored)
{
    if(_nodes.full())
    {
        return bn::optional<int>();
    }

    Node node;
    node.position = bn::fixed_point(x, y);
    node.is_anchored = is_anchored;
    _nodes.push_back(node);

    if(is_anchored)
    {
        ++_anchored_node_count;
    }
    else
    {
        _record_node_edit(_nodes.size() - 1);
    }

    return bn::optional<int>(_nodes.size() - 1);
}

bn::optional<int> BridgeSystem::add_strut(int node_a, int node_b, const LinkRole role,
                                          const MaterialId material)
{
    LinkRole resolved_role = role;
    MaterialId resolved_material = material;

    // Legacy 2-arg callers pass brace/foundation with the default wood material.
    if(resolved_material == MaterialId::wood && resolved_role == LinkRole::brace)
    {
        resolved_material = MaterialId::iron;
    }
    else if(resolved_material == MaterialId::wood && resolved_role == LinkRole::foundation)
    {
        resolved_material = MaterialId::stone;
    }

    if(material::is_pier(resolved_material))
    {
        resolved_role = LinkRole::foundation;
    }
    else if(resolved_role == LinkRole::foundation)
    {
        resolved_material = MaterialId::stone;
    }

    if(!allows_link_role(resolved_role) || !allows_material(resolved_material))
    {
        return bn::optional<int>();
    }

    if(material::is_pier(resolved_material) && !_pier_placement_ok(node_a, node_b))
    {
        return bn::optional<int>();
    }

    if(node_a == node_b)
    {
        return bn::optional<int>();
    }

    if(node_a < 0 || node_b < 0 || node_a >= _nodes.size() || node_b >= _nodes.size())
    {
        return bn::optional<int>();
    }

    if(_struts.full())
    {
        return bn::optional<int>();
    }

    const int lo = min_id(node_a, node_b);
    const int hi = max_id(node_a, node_b);

    if(find_strut(lo, hi))
    {
        return bn::optional<int>();
    }

    const bn::fixed length = physics::distance(_nodes[lo].position, _nodes[hi].position);

    if(length <= 0)
    {
        return bn::optional<int>();
    }

    Strut strut;
    strut.node_a = lo;
    strut.node_b = hi;
    strut.role = resolved_role;
    strut.material = resolved_material;
    strut.length = length;
    strut.current_tension = 0;
    strut.stress = 0;
    strut.triangulated = false;
    strut.max_tension = 0;
    _struts.push_back(strut);
    _recompute_strut_ratings();
    _record_strut_edit(_struts.size() - 1);
    return bn::optional<int>(_struts.size() - 1);
}

bool BridgeSystem::_pier_placement_ok(const int node_a, const int node_b) const
{
    if(node_a < 0 || node_b < 0 || node_a >= _nodes.size() || node_b >= _nodes.size())
    {
        return false;
    }

    const Node& a = _nodes[node_a];
    const Node& b = _nodes[node_b];
    const bool a_ground = a.is_anchored && a.position.y() >= game::TRACK_Y - 4;
    const bool b_ground = b.is_anchored && b.position.y() >= game::TRACK_Y - 4;
    return a_ground || b_ground;
}

int BridgeSystem::_connection_count(int node_id) const
{
    int count = 0;

    for(int i = 0; i < _struts.size(); ++i)
    {
        const Strut& strut = _struts[i];

        if(strut.node_a == node_id || strut.node_b == node_id)
        {
            ++count;
        }
    }

    return count;
}

bool BridgeSystem::_strut_is_triangulated(int strut_index) const
{
    const Strut& strut = _struts[strut_index];

    for(int c = 0; c < _nodes.size(); ++c)
    {
        if(c == strut.node_a || c == strut.node_b)
        {
            continue;
        }

        if(find_strut(strut.node_a, c) && find_strut(strut.node_b, c))
        {
            return true;
        }
    }

    return false;
}

void BridgeSystem::_recompute_strut_ratings()
{
    for(int i = 0; i < _struts.size(); ++i)
    {
        Strut& strut = _struts[i];
        const int connections_a = _connection_count(strut.node_a);
        const int connections_b = _connection_count(strut.node_b);
        strut.triangulated = _strut_is_triangulated(i);
        strut.max_tension = physics::rated_yield(strut.material, strut.length, connections_a,
                                                 connections_b, strut.triangulated, _safe_length);
    }
}

bn::optional<int> BridgeSystem::find_node_near(bn::fixed_point point) const
{
    const bn::fixed snap2 = SNAP_RADIUS.safe_multiplication(SNAP_RADIUS);
    bn::optional<int> best;
    bn::fixed best_d2 = snap2;

    for(int i = 0; i < _nodes.size(); ++i)
    {
        const bn::fixed d2 = physics::squared_distance(point, _nodes[i].position);

        if(d2 <= best_d2)
        {
            best_d2 = d2;
            best = i;
        }
    }

    return best;
}

bn::optional<int> BridgeSystem::find_strut_near(bn::fixed_point point) const
{
    const bn::fixed snap2 = SNAP_RADIUS.safe_multiplication(SNAP_RADIUS);
    bn::optional<int> best;
    bn::fixed best_d2 = snap2;

    for(int i = 0; i < _struts.size(); ++i)
    {
        const Strut& strut = _struts[i];
        const bn::fixed_point a = _nodes[strut.node_a].position;
        const bn::fixed_point b = _nodes[strut.node_b].position;
        const bn::fixed_point mid((a.x() + b.x()) / 2, (a.y() + b.y()) / 2);
        const bn::fixed d2 = physics::squared_distance(point, mid);

        if(d2 <= best_d2)
        {
            best_d2 = d2;
            best = i;
        }
    }

    return best;
}

StrutPreview BridgeSystem::preview_strut(int node_a, int node_b, const LinkRole role,
                                         const MaterialId material) const
{
    StrutPreview preview;
    const MaterialId mat = material;
    const LinkRole resolved =
        material::is_pier(mat) ? LinkRole::foundation : role;

    if(!allows_link_role(resolved) || !allows_material(mat))
    {
        preview.status = StrutPreviewStatus::invalid_node;
        return preview;
    }

    if(material::is_pier(mat) && !_pier_placement_ok(node_a, node_b))
    {
        preview.status = StrutPreviewStatus::invalid_node;
        return preview;
    }

    if(node_a == node_b)
    {
        preview.status = StrutPreviewStatus::same_node;
        return preview;
    }

    if(node_a < 0 || node_b < 0 || node_a >= _nodes.size() || node_b >= _nodes.size())
    {
        preview.status = StrutPreviewStatus::invalid_node;
        return preview;
    }

    if(_struts.full())
    {
        preview.status = StrutPreviewStatus::struts_full;
        return preview;
    }

    const int lo = min_id(node_a, node_b);
    const int hi = max_id(node_a, node_b);

    if(find_strut(lo, hi))
    {
        preview.status = StrutPreviewStatus::duplicate;
        return preview;
    }

    const bn::fixed length = physics::distance(_nodes[lo].position, _nodes[hi].position);

    if(length <= 0)
    {
        preview.status = StrutPreviewStatus::zero_length;
        return preview;
    }

    preview.status = StrutPreviewStatus::ok;
    preview.length = length;

    const int connections_a = _connection_count(lo) + 1;
    const int connections_b = _connection_count(hi) + 1;

    bool triangulated = false;

    for(int c = 0; c < _nodes.size(); ++c)
    {
        if(c == lo || c == hi)
        {
            continue;
        }

        if(find_strut(lo, c) && find_strut(hi, c))
        {
            triangulated = true;
            break;
        }
    }

    preview.max_tension =
        physics::rated_yield(mat, length, connections_a, connections_b, triangulated, _safe_length);
    return preview;
}

bn::fixed BridgeSystem::span_preview_capacity(const bn::fixed length, const LinkRole role,
                                              const MaterialId material) const
{
    if(length <= 0 || !allows_link_role(role) || !allows_material(material))
    {
        return 0;
    }

    return physics::rated_yield(material, length, 1, 1, false, _safe_length);
}

bool BridgeSystem::remove_strut(const int strut_index)
{
    if(_status != SimStatus::idle)
    {
        return false;
    }

    if(strut_index < 0 || strut_index >= _struts.size())
    {
        return false;
    }

    _remove_strut_at(strut_index);
    _recompute_strut_ratings();
    _last_edit = LastEdit::none;
    _last_edit_index = -1;
    return true;
}

int BridgeSystem::budget_spent() const
{
    int spent = player_node_count() * NODE_COST;

    for(const Strut& strut : _struts)
    {
        spent += link_cost(strut.material);
    }

    return spent;
}

int BridgeSystem::budget_remaining() const
{
    const int purse = level::effective_budget(level_def());
    const int spent = budget_spent();
    return spent >= purse ? 0 : purse - spent;
}

bn::fixed BridgeSystem::peak_stress() const
{
    bn::fixed peak = 0;

    for(const Strut& strut : _struts)
    {
        if(strut.stress > peak)
        {
            peak = strut.stress;
        }
    }

    return peak;
}

UndoResult BridgeSystem::undo_last_edit()
{
    if(_status != SimStatus::idle)
    {
        return UndoResult::nothing;
    }

    switch(_last_edit)
    {
        case LastEdit::none:
            return UndoResult::nothing;

        case LastEdit::strut:
        {
            if(_last_edit_index < 0 || _last_edit_index >= _struts.size())
            {
                _last_edit = LastEdit::none;
                _last_edit_index = -1;
                return UndoResult::nothing;
            }

            _remove_strut_at(_last_edit_index);
            _last_edit = LastEdit::none;
            _last_edit_index = -1;
            return UndoResult::removed_strut;
        }

        case LastEdit::node:
        {
            if(_last_edit_index < 0 || _last_edit_index >= _nodes.size() ||
               _nodes[_last_edit_index].is_anchored)
            {
                _last_edit = LastEdit::none;
                _last_edit_index = -1;
                return UndoResult::nothing;
            }

            if(remove_player_node(_last_edit_index) == RemoveNodeResult::ok)
            {
                _last_edit = LastEdit::none;
                _last_edit_index = -1;
                return UndoResult::removed_node;
            }

            return UndoResult::nothing;
        }

        default:
            return UndoResult::nothing;
    }
}

RemoveNodeResult BridgeSystem::remove_player_node(int node_id)
{
    if(_status != SimStatus::idle)
    {
        return RemoveNodeResult::in_test;
    }

    if(node_id < 0 || node_id >= _nodes.size())
    {
        return RemoveNodeResult::invalid;
    }

    if(_nodes[node_id].is_anchored)
    {
        return RemoveNodeResult::anchored;
    }

    for(int i = _struts.size() - 1; i >= 0; --i)
    {
        const Strut& strut = _struts[i];

        if(strut.node_a == node_id || strut.node_b == node_id)
        {
            _remove_strut_at(i);
        }
    }

    for(int i = node_id + 1; i < _nodes.size(); ++i)
    {
        _nodes[i - 1] = _nodes[i];
    }

    _nodes.pop_back();

    for(int i = 0; i < _struts.size(); ++i)
    {
        Strut& strut = _struts[i];

        if(strut.node_a > node_id)
        {
            --strut.node_a;
        }

        if(strut.node_b > node_id)
        {
            --strut.node_b;
        }
    }

    if(_last_edit == LastEdit::node)
    {
        if(_last_edit_index == node_id)
        {
            _last_edit = LastEdit::none;
            _last_edit_index = -1;
        }
        else if(_last_edit_index > node_id)
        {
            --_last_edit_index;
        }
    }

    _recompute_strut_ratings();
    return RemoveNodeResult::ok;
}

bool BridgeSystem::allows_link_role(const LinkRole role) const
{
    return level::allows_role(level::get(_level_index), link_role_bit(role));
}

bool BridgeSystem::allows_material(const MaterialId mat) const
{
    return level::allows_material(level::get(_level_index), link_material_bit(mat));
}

bool BridgeSystem::is_player_node(int node_id) const
{
    return node_id >= 0 && node_id < _nodes.size() && !_nodes[node_id].is_anchored;
}

bool BridgeSystem::has_track_path() const
{
    bool visited[MAX_NODES];

    for(int i = 0; i < MAX_NODES; ++i)
    {
        visited[i] = false;
    }

    bn::vector<int, MAX_NODES> queue;
    queue.push_back(START_NODE);
    visited[START_NODE] = true;

    while(!queue.empty())
    {
        const int current = queue[0];
        queue.erase(queue.begin());

        if(current == END_NODE)
        {
            return true;
        }

        for(int s = 0; s < _struts.size(); ++s)
        {
            const Strut& strut = _struts[s];

            if(!is_routable(strut.role))
            {
                continue;
            }

            int next = -1;

            if(strut.node_a == current)
            {
                next = strut.node_b;
            }
            else if(strut.node_b == current)
            {
                next = strut.node_a;
            }

            if(next >= 0 && !visited[next])
            {
                visited[next] = true;
                queue.push_back(next);
            }
        }
    }

    return false;
}

void BridgeSystem::_record_node_edit(int node_id)
{
    _last_edit = LastEdit::node;
    _last_edit_index = node_id;
}

void BridgeSystem::_record_strut_edit(int strut_index)
{
    _last_edit = LastEdit::strut;
    _last_edit_index = strut_index;
}

void BridgeSystem::_remove_strut_at(int strut_index)
{
    for(int i = strut_index + 1; i < _struts.size(); ++i)
    {
        _struts[i - 1] = _struts[i];
    }

    _struts.pop_back();

    if(_last_edit == LastEdit::strut)
    {
        if(_last_edit_index == strut_index)
        {
            _last_edit = LastEdit::none;
            _last_edit_index = -1;
        }
        else if(_last_edit_index > strut_index)
        {
            --_last_edit_index;
        }
    }

    _recompute_strut_ratings();
}

bn::optional<int> BridgeSystem::find_strut(int node_a, int node_b) const
{
    const int lo = min_id(node_a, node_b);
    const int hi = max_id(node_a, node_b);

    for(int i = 0; i < _struts.size(); ++i)
    {
        if(_struts[i].node_a == lo && _struts[i].node_b == hi)
        {
            return i;
        }
    }

    return bn::optional<int>();
}

bool BridgeSystem::_bfs_path()
{
    _path.clear();

    int parent[MAX_NODES];
    bool visited[MAX_NODES];

    for(int i = 0; i < MAX_NODES; ++i)
    {
        parent[i] = -1;
        visited[i] = false;
    }

    bn::vector<int, MAX_NODES> queue;
    queue.push_back(START_NODE);
    visited[START_NODE] = true;

    bool found = false;

    while(!queue.empty() && !found)
    {
        const int current = queue[0];
        queue.erase(queue.begin());

        if(current == END_NODE)
        {
            found = true;
            break;
        }

        for(int s = 0; s < _struts.size(); ++s)
        {
            const Strut& strut = _struts[s];

            if(!is_routable(strut.role))
            {
                continue;
            }

            int next = -1;

            if(strut.node_a == current)
            {
                next = strut.node_b;
            }
            else if(strut.node_b == current)
            {
                next = strut.node_a;
            }

            if(next >= 0 && !visited[next])
            {
                visited[next] = true;
                parent[next] = current;
                queue.push_back(next);
            }
        }
    }

    if(!visited[END_NODE])
    {
        return false;
    }

    bn::vector<int, MAX_NODES> reverse;
    int cursor = END_NODE;

    while(cursor != -1)
    {
        reverse.push_back(cursor);
        cursor = parent[cursor];
    }

    for(int i = reverse.size() - 1; i >= 0; --i)
    {
        _path.push_back(reverse[i]);
    }

    return _path.size() >= 2;
}

SimStatus BridgeSystem::begin_test()
{
    _saved_struts = _struts;
    _recompute_strut_ratings();
    _falling = false;
    _fall_vy = 0;
    _train_t = 0;
    _path_index = 0;
    _accel_frames = 0;
    _train_position = _nodes[START_NODE].position;
    _train_state = TrainState::waiting;

    for(int i = 0; i < _struts.size(); ++i)
    {
        _struts[i].current_tension = 0;
        _struts[i].stress = 0;
    }

    if(!_bfs_path())
    {
        _status = SimStatus::no_path;
        return _status;
    }

    _status = SimStatus::running;
    _train_state = TrainState::crossing;
    return _status;
}

void BridgeSystem::end_test()
{
    if(!_saved_struts.empty() || _status != SimStatus::idle)
    {
        _struts = _saved_struts;
        _saved_struts.clear();
    }

    _path.clear();
    _status = SimStatus::idle;
    _train_state = TrainState::waiting;
    _falling = false;
    _fall_vy = 0;
    _train_t = 0;
    _path_index = 0;
    _accel_frames = 0;
    _train_position = _nodes[START_NODE].position;

    for(int i = 0; i < _struts.size(); ++i)
    {
        _struts[i].current_tension = 0;
        _struts[i].stress = 0;
    }
}

void BridgeSystem::_break_strut(int strut_index)
{
    const Strut& broken = _struts[strut_index];
    _snap_flash = SnapFlash{
        _nodes[broken.node_a].position,
        _nodes[broken.node_b].position,
        12,
    };
    _struts.erase(_struts.begin() + strut_index);
    _recompute_strut_ratings();
}

void BridgeSystem::_clear_member_forces()
{
    for(int i = 0; i < _struts.size(); ++i)
    {
        _struts[i].current_tension = 0;
        _struts[i].stress = 0;
    }
}

void BridgeSystem::_solve_truss_stress(const int active_strut, const bn::fixed distance_along)
{
    _clear_member_forces();

    if(active_strut < 0 || active_strut >= _struts.size())
    {
        return;
    }

    Strut& deck = _struts[active_strut];
    const bn::fixed load = effective_weight_at(distance_along, deck.length);
    // Primary axial force on the loaded track member (legacy tension curve).
    bn::fixed deck_force = physics::tension(load, distance_along, deck.length);

    if(material::is_tension_only(deck.material) && deck_force < 0)
    {
        deck_force = 0;
    }

    deck.current_tension = deck_force;
    deck.stress = physics::stress_ratio(deck.current_tension, deck.max_tension);

    // Redistribute a share of the joint load into braces / piers / cables
    // incident on the active endpoints (relative EA/L stiffness).
    const int ends[2] = { deck.node_a, deck.node_b };
    const bn::fixed share_scale = bn::fixed(1) / 2;

    for(int e = 0; e < 2; ++e)
    {
        const int node = ends[e];
        bn::fixed stiffness_sum = 0;

        for(int i = 0; i < _struts.size(); ++i)
        {
            if(i == active_strut)
            {
                continue;
            }

            const Strut& s = _struts[i];

            if(s.node_a != node && s.node_b != node)
            {
                continue;
            }

            stiffness_sum += physics::member_stiffness(s.material, s.length);
        }

        if(stiffness_sum <= 0)
        {
            continue;
        }

        const bn::fixed joint_load = load.safe_multiplication(share_scale);

        for(int i = 0; i < _struts.size(); ++i)
        {
            if(i == active_strut)
            {
                continue;
            }

            Strut& s = _struts[i];

            if(s.node_a != node && s.node_b != node)
            {
                continue;
            }

            const bn::fixed k = physics::member_stiffness(s.material, s.length);
            bn::fixed force = joint_load.safe_multiplication(k).safe_division(stiffness_sum);

            if(material::is_tension_only(s.material))
            {
                // Cables only carry tension; treat redistributed vertical share as tension.
                // Compression contribution is discarded (force already >= 0 here).
            }

            s.current_tension += force;
            s.stress = physics::stress_ratio(s.current_tension, s.max_tension);
        }
    }
}

bool BridgeSystem::_cascade_collapse(const int /*active_strut*/)
{
    bool broke_active = false;
    const int from = (_path_index < _path.size()) ? _path[_path_index] : -1;
    const int to = (_path_index + 1 < _path.size()) ? _path[_path_index + 1] : -1;

    for(int pass = 0; pass < physics::CASCADE_MAX_PASSES; ++pass)
    {
        int worst = -1;
        bn::fixed worst_stress = 0;

        for(int i = 0; i < _struts.size(); ++i)
        {
            if(physics::is_breaking(_struts[i].stress) && _struts[i].stress > worst_stress)
            {
                worst_stress = _struts[i].stress;
                worst = i;
            }
        }

        if(worst < 0)
        {
            break;
        }

        const Strut& doomed = _struts[worst];
        const bool on_path =
            from >= 0 && to >= 0 &&
            ((doomed.node_a == from && doomed.node_b == to) ||
             (doomed.node_b == from && doomed.node_a == to));

        if(on_path)
        {
            broke_active = true;
        }

        _break_strut(worst);

        if(broke_active)
        {
            break;
        }

        const bn::optional<int> still = find_strut(from, to);

        if(!still)
        {
            broke_active = true;
            break;
        }

        const bn::fixed along = _train_t.safe_multiplication(_struts[still.value()].length);
        _solve_truss_stress(still.value(), along);
    }

    return broke_active;
}

const SnapFlash* BridgeSystem::snap_flash() const
{
    return _snap_flash.has_value() ? &_snap_flash.value() : nullptr;
}

void BridgeSystem::_start_falling()
{
    _falling = true;
    _fall_vy = FALL_INITIAL_VY;
    _status = SimStatus::failed_fall;
    _train_state = TrainState::derailed;
}

bn::fixed BridgeSystem::_current_train_load() const
{
    return _path_index == 0 ? physics::ENGINE_WEIGHT : physics::BASE_WEIGHT;
}

bn::fixed BridgeSystem::_current_train_speed() const
{
    if(_accel_frames >= ACCEL_FRAMES)
    {
        return _train_speed;
    }

    const bn::fixed t = bn::fixed(_accel_frames).safe_division(ACCEL_FRAMES);
    return ENGINE_MIN_SPEED + (_train_speed - ENGINE_MIN_SPEED).safe_multiplication(t);
}

bn::fixed BridgeSystem::current_train_speed() const
{
    return _current_train_speed();
}

void BridgeSystem::_apply_tension(int strut_index, bn::fixed distance_along)
{
    _solve_truss_stress(strut_index, distance_along);
}

SimStatus BridgeSystem::update()
{
    if(_snap_flash.has_value())
    {
        SnapFlash& flash = _snap_flash.value();
        --flash.frames;

        if(flash.frames <= 0)
        {
            _snap_flash.reset();
        }
    }

    if(_status == SimStatus::failed_fall)
    {
        _fall_vy += GRAVITY;
        _train_position.set_y(_train_position.y() + _fall_vy);

        if(_train_position.y() > FALL_Y_LIMIT)
        {
            _train_position.set_y(FALL_Y_LIMIT);
        }

        return _status;
    }

    if(_status != SimStatus::running)
    {
        return _status;
    }

    if(_path_index + 1 >= _path.size())
    {
        _status = SimStatus::success;
        _train_state = TrainState::arrived;
        _train_position = _nodes[END_NODE].position;
        return _status;
    }

    const int from = _path[_path_index];
    const int to = _path[_path_index + 1];
    const bn::optional<int> strut_id = find_strut(from, to);

    if(!strut_id)
    {
        _start_falling();
        return _status;
    }

    const int strut_index = strut_id.value();
    const Node& a = _nodes[from];
    const Node& b = _nodes[to];
    const bn::fixed length = _struts[strut_index].length;
    const bn::fixed speed = _current_train_speed();
    const bn::fixed dt = length > 0 ? speed.safe_division(length) : bn::fixed(1);

    _train_t += dt;
    ++_accel_frames;

    bn::fixed along = _train_t.safe_multiplication(length);

    if(along > length)
    {
        along = length;
    }

    _apply_tension(strut_index, along);

    if(_cascade_collapse(strut_index))
    {
        _train_position = physics::lerp(a.position, b.position, _train_t);
        _start_falling();
        return _status;
    }

    if(_train_t >= 1)
    {
        _train_t = 0;
        _path_index += 1;
        _train_position = b.position;
        _clear_member_forces();

        if(_path_index + 1 >= _path.size())
        {
            _status = SimStatus::success;
            _train_state = TrainState::arrived;
        }
    }
    else
    {
        _train_position = physics::lerp(a.position, b.position, _train_t);
    }

    return _status;
}

} // namespace bridge
