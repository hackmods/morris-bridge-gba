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
    _rebuild_train();
}

void BridgeSystem::_rebuild_train()
{
    const engine::Profile& profile = engine::profile(_engine);
    _cars.clear();

    for(int i = 0; i < profile.car_count; ++i)
    {
        _cars.push_back(profile.cars[i]);
    }

    if(level::get(_level_index).banked)
    {
        _cars.push_back(engine::Car{ engine::CarKind::banker, engine::BANKER_WEIGHT });
    }
}

bn::fixed BridgeSystem::train_total_weight() const
{
    bn::fixed total = _train_weight;

    for(const engine::Car& car : _cars)
    {
        total += car.weight;
    }

    return total;
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
    _beam_length = def.safe_length > 0 ? def.safe_length : level::DEFAULT_BEAM_LENGTH;
    _rebuild_train();
    reset_level();
}

void BridgeSystem::reset_level()
{
    _nodes.clear();
    _struts.clear();
    _saved_nodes.clear();
    _saved_struts.clear();
    _path.clear();
    _status = SimStatus::idle;
    _train_distance = 0;
    _train_t = 0;
    _path_index = 0;
    _accel_frames = 0;
    _settle_frames = 0;
    _test_frames = 0;
    _snapped_count = 0;
    _fall_vy = 0;
    _falling = false;
    _test_ran = false;
    _snap_flash.reset();
    _last_edit = LastEdit::none;
    _last_edit_index = -1;
    _seed_level();
    _park_train();
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

bn::optional<int> BridgeSystem::add_strut(int node_a, int node_b, const LinkRole role)
{
    const StrutPreview preview = preview_strut(node_a, node_b, role);

    if(preview.status != StrutPreviewStatus::ok)
    {
        return bn::optional<int>();
    }

    Strut strut;
    strut.node_a = min_id(node_a, node_b);
    strut.node_b = max_id(node_a, node_b);
    strut.role = role;
    strut.material = material_for_role(role);
    strut.length = preview.length;
    strut.max_tension = material(strut.material).strength;
    _struts.push_back(strut);
    _record_strut_edit(_struts.size() - 1);
    return bn::optional<int>(_struts.size() - 1);
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

StrutPreview BridgeSystem::preview_strut(int node_a, int node_b, const LinkRole role) const
{
    StrutPreview preview;

    if(!allows_link_role(role))
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
    preview.length = length;
    preview.max_length = max_link_length(role);

    if(length < MIN_LINK_LENGTH)
    {
        preview.status = StrutPreviewStatus::zero_length;
        return preview;
    }

    if(length > preview.max_length)
    {
        preview.status = StrutPreviewStatus::too_long;
        return preview;
    }

    preview.status = StrutPreviewStatus::ok;
    return preview;
}

bn::fixed BridgeSystem::max_link_length(const LinkRole role) const
{
    const Material& mat = material(material_for_role(role));
    return _beam_length * mat.reach_num / mat.reach_den;
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
        {
            // Nothing fresh to take back: keep unwinding in build order. A
            // joint is welded before the beam that reaches it, so the
            // newest thing is the last beam, or the last joint if it has
            // no beam left.
            const int last_node = _nodes.size() - 1;

            if(last_node >= 0 && is_player_node(last_node))
            {
                bool attached = false;

                for(const Strut& strut : _struts)
                {
                    attached = attached || strut.node_a == last_node || strut.node_b == last_node;
                }

                if(! attached)
                {
                    return remove_player_node(last_node) == RemoveNodeResult::ok ?
                        UndoResult::removed_node : UndoResult::nothing;
                }
            }

            if(_struts.empty())
            {
                return UndoResult::nothing;
            }

            _remove_strut_at(_struts.size() - 1);
            return UndoResult::removed_strut;
        }

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

    return RemoveNodeResult::ok;
}

bool BridgeSystem::allows_link_role(const LinkRole role) const
{
    return level::allows_role(level::get(_level_index), link_role_bit(role));
}

bool BridgeSystem::is_player_node(int node_id) const
{
    return node_id >= 0 && node_id < _nodes.size() && !_nodes[node_id].is_anchored;
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

bn::optional<int> BridgeSystem::find_strut_near(const bn::fixed_point point) const
{
    bn::optional<int> best;
    bn::fixed best_d2 = LINK_PICK_RADIUS * LINK_PICK_RADIUS;

    for(int i = 0; i < _struts.size(); ++i)
    {
        const bn::fixed_point& a = _nodes[_struts[i].node_a].position;
        const bn::fixed_point& b = _nodes[_struts[i].node_b].position;
        const bn::fixed_point along = b - a;
        const bn::fixed length2 = physics::squared_distance(a, b);

        if(length2 <= 0)
        {
            continue;
        }

        // Closest point on the segment, clamped to its ends.
        bn::fixed t = ((point.x() - a.x()).safe_multiplication(along.x()) +
                       (point.y() - a.y()).safe_multiplication(along.y()))
                          .safe_division(length2);
        t = t < 0 ? bn::fixed(0) : (t > 1 ? bn::fixed(1) : t);
        const bn::fixed d2 = physics::squared_distance(point, physics::lerp(a, b, t));

        if(d2 <= best_d2)
        {
            best_d2 = d2;
            best = i;
        }
    }

    return best;
}

bool BridgeSystem::remove_strut(const int strut_index)
{
    if(_status != SimStatus::idle || strut_index < 0 || strut_index >= _struts.size())
    {
        return false;
    }

    _remove_strut_at(strut_index);
    return true;
}

int BridgeSystem::stress_percent() const
{
    int worst = 0;

    for(const Strut& strut : _struts)
    {
        const int percent = (strut.current_tension * 100).safe_division(strut.max_tension).integer();

        if(percent > worst)
        {
            worst = percent;
        }
    }

    return worst;
}

bool BridgeSystem::is_routable(const Strut& strut) const
{
    if(strut.role != LinkRole::track)
    {
        return false;
    }

    const bn::fixed_point& a = _nodes[strut.node_a].position;
    const bn::fixed_point& b = _nodes[strut.node_b].position;
    return physics::abs(b.y() - a.y()) * MAX_GRADE_RUN <= physics::abs(b.x() - a.x()) * MAX_GRADE_RISE;
}

bool BridgeSystem::has_track_path(const bool ignore_grade) const
{
    bool visited[MAX_NODES] = {};
    bn::vector<int, MAX_NODES> queue;
    queue.push_back(START_NODE);
    visited[START_NODE] = true;

    for(int head = 0; head < queue.size(); ++head)
    {
        const int current = queue[head];

        if(current == END_NODE)
        {
            return true;
        }

        for(const Strut& strut : _struts)
        {
            if(strut.node_a != current && strut.node_b != current)
            {
                continue;
            }

            const int next = strut.node_a == current ? strut.node_b : strut.node_a;

            if(!visited[next] && strut.role == LinkRole::track && (ignore_grade || is_routable(strut)))
            {
                visited[next] = true;
                queue.push_back(next);
            }
        }
    }

    return false;
}

void BridgeSystem::_mark_climbable(const int from, bool* reached) const
{
    bn::vector<int, MAX_NODES> queue;
    queue.push_back(from);
    reached[from] = true;

    for(int head = 0; head < queue.size(); ++head)
    {
        const int current = queue[head];

        for(const Strut& strut : _struts)
        {
            if((strut.node_a != current && strut.node_b != current) || !is_routable(strut))
            {
                continue;
            }

            const int next = strut.node_a == current ? strut.node_b : strut.node_a;

            if(!reached[next])
            {
                reached[next] = true;
                queue.push_back(next);
            }
        }
    }
}

bn::optional<int> BridgeSystem::steep_strut() const
{
    bool from_start[MAX_NODES] = {};
    bool from_end[MAX_NODES] = {};
    _mark_climbable(START_NODE, from_start);
    _mark_climbable(END_NODE, from_end);

    if(from_start[END_NODE])
    {
        return bn::optional<int>();
    }

    for(int i = 0; i < _struts.size(); ++i)
    {
        const Strut& strut = _struts[i];

        if(strut.role == LinkRole::track && !is_routable(strut) &&
           ((from_start[strut.node_a] && from_end[strut.node_b]) ||
            (from_start[strut.node_b] && from_end[strut.node_a])))
        {
            return bn::optional<int>(i);
        }
    }

    return bn::optional<int>();
}

bool BridgeSystem::_find_route()
{
    // Shortest climbable track route by length (Dijkstra, at most 16 nodes), so
    // a timber truss chord above the deck never steals the train from the deck.
    _path.clear();

    constexpr int UNREACHED = 0x7FFFFFFF;
    int dist[MAX_NODES];
    int parent[MAX_NODES];
    bool done[MAX_NODES] = {};

    for(int i = 0; i < MAX_NODES; ++i)
    {
        dist[i] = UNREACHED;
        parent[i] = -1;
    }

    dist[START_NODE] = 0;

    for(int pass = 0; pass < _nodes.size(); ++pass)
    {
        int current = -1;

        for(int i = 0; i < _nodes.size(); ++i)
        {
            if(!done[i] && dist[i] != UNREACHED && (current < 0 || dist[i] < dist[current]))
            {
                current = i;
            }
        }

        if(current < 0 || current == END_NODE)
        {
            break;
        }

        done[current] = true;

        for(const Strut& strut : _struts)
        {
            if(strut.node_a != current && strut.node_b != current)
            {
                continue;
            }

            const int next = strut.node_a == current ? strut.node_b : strut.node_a;
            const int through = dist[current] + strut.length.data();

            if(!done[next] && through < dist[next] && is_routable(strut))
            {
                dist[next] = through;
                parent[next] = current;
            }
        }
    }

    _route_complete = dist[END_NODE] != UNREACHED;
    _stall_at_end = false;
    int target = END_NODE;

    if(!_route_complete)
    {
        // No way across: the train takes the track as far right as it goes.
        target = START_NODE;

        for(int i = 0; i < _nodes.size(); ++i)
        {
            if(dist[i] != UNREACHED && _nodes[i].position.x() > _nodes[target].position.x())
            {
                target = i;
            }
        }

        // A beam leading on from there that is too steep stops the engine
        // rather than dropping it.
        for(const Strut& strut : _struts)
        {
            if(strut.role != LinkRole::track || is_routable(strut) ||
               (strut.node_a != target && strut.node_b != target))
            {
                continue;
            }

            const int other = strut.node_a == target ? strut.node_b : strut.node_a;
            _stall_at_end = _stall_at_end || _nodes[other].position.x() > _nodes[target].position.x();
        }
    }

    bn::vector<int, MAX_NODES> reverse;

    for(int cursor = target; cursor != -1; cursor = parent[cursor])
    {
        reverse.push_back(cursor);
    }

    for(int i = reverse.size() - 1; i >= 0; --i)
    {
        _path_cum[_path.size()] = bn::fixed::from_data(dist[reverse[i]]);
        _path.push_back(reverse[i]);
    }

    return _route_complete;
}

void BridgeSystem::_prepare_solver()
{
    for(int i = 0; i < _nodes.size(); ++i)
    {
        _sim[i] = NodeSim();
        _sim[i].weight = _nodes[i].is_anchored ? 0 : JOINT_WEIGHT.data();
        _load[i] = 0;
    }

    for(Strut& strut : _struts)
    {
        const int64_t rest = strut.length.data();
        const Material& mat = material(strut.material);

        strut.inv_rest2 = int32_t((int64_t(1) << 56) / (rest * rest));
        strut.inv_rest = int32_t((int64_t(1) << 32) / rest);
        strut.ea_q4 = mat.stiffness.data() >> (bn::fixed::precision() - 4);
        strut.tension_only = mat.tension_only;
        strut.max_tension = mat.strength;
        strut.current_tension = 0;
        strut.peak_tension = 0;
        strut.compressed = false;

        // Each joint carries half of every link that meets it.
        const int32_t half = strut.length.safe_multiplication(mat.weight_per_px).data() / 2;

        if(!_nodes[strut.node_a].is_anchored)
        {
            _sim[strut.node_a].weight += half;
        }

        if(!_nodes[strut.node_b].is_anchored)
        {
            _sim[strut.node_b].weight += half;
        }
    }

    _refresh_inertia();
}

void BridgeSystem::_refresh_inertia()
{
    // Explicit integration is only stable while (stiffness / inertia) at every
    // joint stays under the step limit. Joints keep their true weight for
    // gravity but borrow extra inertia where stiff links meet, which leaves
    // the static force balance — what decides a break — untouched.
    constexpr int STABILITY_DIVISOR = 32;
    bn::fixed stiffness_sum[MAX_NODES] = {};

    for(const Strut& strut : _struts)
    {
        const bn::fixed per_px = material(strut.material).stiffness.safe_division(strut.length);
        stiffness_sum[strut.node_a] += per_px;
        stiffness_sum[strut.node_b] += per_px;
    }

    for(int i = 0; i < _nodes.size(); ++i)
    {
        if(_nodes[i].is_anchored)
        {
            _sim[i].inv_inertia = 0;
            continue;
        }

        bn::fixed inertia = bn::fixed::from_data(_sim[i].weight);
        const bn::fixed needed = stiffness_sum[i] / STABILITY_DIVISOR;

        if(needed > inertia)
        {
            inertia = needed;
        }

        _sim[i].inv_inertia = bn::fixed(1).safe_division(inertia).data();
    }
}

SimStatus BridgeSystem::begin_test()
{
    _falling = false;
    _fall_vy = 0;
    _train_distance = 0;
    _train_t = 0;
    _path_index = 0;
    _accel_frames = 0;
    _test_frames = 0;
    _snapped_count = 0;
    _peak_stress_percent = 0;
    _snap_flash.reset();
    _park_train();
    _ran_off = false;

    for(bn::fixed& speed : _axle_fall)
    {
        speed = 0;
    }

    (void)_find_route();

    _saved_nodes = _nodes;
    _saved_struts = _struts;
    _test_ran = true;
    _settle_frames = SETTLE_FRAMES;
    _prepare_solver();
    _status = SimStatus::running;
    return _status;
}

void BridgeSystem::end_test()
{
    if(_test_ran)
    {
        // The rewind: the build comes back exactly as welded, but every link
        // remembers the worst force it carried (a snapped link reads as full).
        for(Strut& saved : _saved_struts)
        {
            const bn::optional<int> live = find_strut(saved.node_a, saved.node_b);
            saved.peak_tension = live ? _struts[live.value()].peak_tension : saved.max_tension;
            saved.current_tension = 0;
            saved.compressed = false;
        }

        _nodes = _saved_nodes;
        _struts = _saved_struts;
        _saved_nodes.clear();
        _saved_struts.clear();
        _test_ran = false;
    }

    _path.clear();
    _status = SimStatus::idle;
    _falling = false;
    _fall_vy = 0;
    _train_distance = 0;
    _train_t = 0;
    _path_index = 0;
    _accel_frames = 0;
    _settle_frames = 0;
    _park_train();
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
    ++_snapped_count;
    _refresh_inertia();
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

void BridgeSystem::_apply_axle(const bn::fixed distance, const bn::fixed weight)
{
    const int last = _path.size() - 1;

    // Axles still on the start bank or already on the far bank load solid ground.
    if(distance <= 0 || distance >= _path_cum[last])
    {
        return;
    }

    const int segment = _segment_at(distance);
    const int a = _path[segment];
    const int b = _path[segment + 1];

    if(!find_strut(a, b))
    {
        return;
    }

    const bn::fixed span = _path_cum[segment + 1] - _path_cum[segment];
    const bn::fixed t = (distance - _path_cum[segment]).safe_division(span);
    const int32_t on_b = weight.safe_multiplication(t).data();
    _load[a] += weight.data() - on_b;
    _load[b] += on_b;
}

bn::fixed BridgeSystem::_axle_distance(const int axle) const
{
    return _train_distance - engine::CAR_SPACING * axle;
}

bn::fixed_point BridgeSystem::axle_heading(const int axle) const
{
    const int last = _path.size() - 1;
    const bn::fixed distance = _axle_distance(axle);

    if(last < 1 || distance <= 0 || distance >= _path_cum[last])
    {
        return bn::fixed_point(1, 0);
    }

    const int segment = _segment_at(distance);
    return _nodes[_path[segment + 1]].position - _nodes[_path[segment]].position;
}

int BridgeSystem::_segment_at(const bn::fixed distance) const
{
    const int last = _path.size() - 1;
    int segment = 0;

    while(segment + 1 < last && distance >= _path_cum[segment + 1])
    {
        ++segment;
    }

    return segment;
}

void BridgeSystem::_park_train()
{
    // Engine at the start anchor, coaches strung out behind it on the bank.
    const bn::fixed_point start = _nodes[START_NODE].position;

    for(int axle = 0; axle <= engine::MAX_CARS; ++axle)
    {
        _axle_position[axle] =
            bn::fixed_point(start.x() - engine::CAR_SPACING * axle, start.y());
    }
}

bn::fixed BridgeSystem::_axle_weight(const int axle) const
{
    if(axle == 0)
    {
        return _train_weight;
    }

    const engine::Car& car = _cars[axle - 1];

    if(car.kind != engine::CarKind::truck)
    {
        return car.weight;
    }

    // Troublesome Trucks never ride smoothly: each one slams down, then goes
    // light, out of step with the truck next to it.
    const bool heavy = ((_test_frames / TRUCK_JOLT_FRAMES) + axle) % 2 == 0;
    return car.weight + (heavy ? car.weight.safe_multiplication(TRUCK_JOLT)
                               : -car.weight.safe_multiplication(TRUCK_JOLT));
}

bn::fixed BridgeSystem::_grade_factor() const
{
    // Engines labour uphill and run away downhill: speed scales with the
    // grade of the link under the engine, sag included.
    const int last = _path.size() - 1;

    if(_train_distance <= 0 || _train_distance >= _path_cum[last])
    {
        return 1;
    }

    const int segment = _segment_at(_train_distance);
    const bn::fixed_point& a = _nodes[_path[segment]].position;
    const bn::fixed_point& b = _nodes[_path[segment + 1]].position;
    const bn::fixed run = physics::abs(b.x() - a.x());

    if(run < 1)
    {
        return 1;
    }

    bn::fixed factor = bn::fixed(1) - (a.y() - b.y()).safe_division(run);

    if(factor < bn::fixed(1) / 2)
    {
        factor = bn::fixed(1) / 2;
    }
    else if(factor > bn::fixed(5) / 4)
    {
        factor = bn::fixed(5) / 4;
    }

    return factor;
}

void BridgeSystem::_apply_train_load()
{
    for(int i = 0; i < _nodes.size(); ++i)
    {
        _load[i] = 0;
    }

    for(int axle = 0; axle < axle_count(); ++axle)
    {
        _apply_axle(_axle_distance(axle), _axle_weight(axle));
    }
}

void BridgeSystem::_substep(const int gravity_q8, const int damping_shift)
{
    constexpr int32_t MAX_VELOCITY = 3 << 20;  // 3 px per substep
    constexpr int32_t FLOOR_Y = 120 << 12;     // well below the screen

    for(int i = 0; i < _nodes.size(); ++i)
    {
        _sim[i].fx = 0;
        _sim[i].fy = ((_sim[i].weight * gravity_q8) >> 8) + _load[i];
    }

    for(int s = 0; s < _struts.size();)
    {
        Strut& strut = _struts[s];
        const bn::fixed_point& a = _nodes[strut.node_a].position;
        const bn::fixed_point& b = _nodes[strut.node_b].position;
        const int32_t dx = b.x().data() - a.x().data();
        const int32_t dy = b.y().data() - a.y().data();
        const int64_t rest = strut.length.data();
        const int64_t rest2 = rest * rest;
        int64_t diff = int64_t(dx) * dx + int64_t(dy) * dy - rest2;

        // x = d²/rest² − 1 in Q16; clamped far past any strain that survives.
        if(diff > rest2 / 2)
        {
            diff = rest2 / 2;
        }
        else if(diff < -(rest2 / 2))
        {
            diff = -(rest2 / 2);
        }

        const int32_t x = int32_t((diff * strut.inv_rest2) >> 40);

        // strain = sqrt(1 + x) − 1 ≈ x/2 − x²/8 + x³/16 — no sqrt, no divide.
        const int32_t x2 = (x * x) >> 16;
        const int32_t strain = (x >> 1) - (x2 >> 3) + (((x2 * x) >> 16) >> 4);
        int32_t force = (strut.ea_q4 * strain) >> 8;  // + tension, − compression

        if(strut.tension_only && force < 0)
        {
            // A cable pushed shorter than its length just hangs slack.
            force = 0;
        }

        const int32_t magnitude = force < 0 ? -force : force;

        strut.current_tension = bn::fixed::from_data(magnitude);
        strut.compressed = force < 0;

        if(strut.current_tension > strut.peak_tension)
        {
            strut.peak_tension = strut.current_tension;
        }

        if(strut.current_tension > strut.max_tension)
        {
            _break_strut(s);
            continue;
        }

        const int32_t ux = int32_t((int64_t(dx) * strut.inv_rest) >> 20);
        const int32_t uy = int32_t((int64_t(dy) * strut.inv_rest) >> 20);
        const int32_t fx = (force * ux) >> 12;
        const int32_t fy = (force * uy) >> 12;
        _sim[strut.node_a].fx += fx;
        _sim[strut.node_a].fy += fy;
        _sim[strut.node_b].fx -= fx;
        _sim[strut.node_b].fy -= fy;
        ++s;
    }

    for(int i = 0; i < _nodes.size(); ++i)
    {
        Node& node = _nodes[i];

        if(node.is_anchored)
        {
            continue;
        }

        NodeSim& sim = _sim[i];
        sim.vx += int32_t((int64_t(sim.fx) * sim.inv_inertia) >> 10);
        sim.vy += int32_t((int64_t(sim.fy) * sim.inv_inertia) >> 10);
        sim.vx -= sim.vx >> damping_shift;
        sim.vy -= sim.vy >> damping_shift;

        if(sim.vx > MAX_VELOCITY)
        {
            sim.vx = MAX_VELOCITY;
        }
        else if(sim.vx < -MAX_VELOCITY)
        {
            sim.vx = -MAX_VELOCITY;
        }

        if(sim.vy > MAX_VELOCITY)
        {
            sim.vy = MAX_VELOCITY;
        }
        else if(sim.vy < -MAX_VELOCITY)
        {
            sim.vy = -MAX_VELOCITY;
        }

        const int32_t px = node.position.x().data() + ((sim.vx + 128) >> 8);
        int32_t py = node.position.y().data() + ((sim.vy + 128) >> 8);

        if(py > FLOOR_Y)
        {
            py = FLOOR_Y;
            sim.vx = 0;
            sim.vy = 0;
        }

        node.position = bn::fixed_point(bn::fixed::from_data(px), bn::fixed::from_data(py));
    }
}

void BridgeSystem::_place_train()
{
    const int last = _path.size() - 1;
    const bn::fixed total = _path_cum[last];
    const bn::fixed_point start = _nodes[START_NODE].position;
    const bn::fixed_point end = _nodes[END_NODE].position;

    if(!_route_complete)
    {
        if(_train_distance >= total && !settling())
        {
            if(_stall_at_end)
            {
                _train_distance = total;
                _status = SimStatus::stalled;
            }
            else
            {
                _ran_off = true;
                _start_falling();
                _run_off_end();
                return;
            }
        }
    }
    else if(_axle_distance(axle_count() - 1) >= total)
    {
        // The run is won when the last coach is off the bridge, not the engine.
        _status = SimStatus::success;
    }

    for(int axle = 0; axle < axle_count(); ++axle)
    {
        const bn::fixed distance = _axle_distance(axle);

        if(distance <= 0)
        {
            // Still on the start bank.
            _axle_position[axle] = bn::fixed_point(start.x() + distance, start.y());
            continue;
        }

        if(distance >= total)
        {
            // Rolled off onto the far bank (or standing at a dead end).
            const bn::fixed_point last_rail = _route_complete ? end : _nodes[_path[last]].position;
            _axle_position[axle] =
                bn::fixed_point(last_rail.x() + (_route_complete ? distance - total : bn::fixed(0)),
                                last_rail.y());
            continue;
        }

        const int segment = _segment_at(distance);
        const int a = _path[segment];
        const int b = _path[segment + 1];
        const bn::fixed span = _path_cum[segment + 1] - _path_cum[segment];
        const bn::fixed t = (distance - _path_cum[segment]).safe_division(span);

        if(axle == 0)
        {
            _path_index = segment;
            _train_t = t;
        }

        if(!find_strut(a, b))
        {
            // The road under any part of the train is gone.
            _start_falling();
            return;
        }

        _axle_position[axle] = physics::lerp(_nodes[a].position, _nodes[b].position, t);

        const bn::fixed built_y =
            physics::lerp(_saved_nodes[a].position, _saved_nodes[b].position, t).y();

        if(_axle_position[axle].y() > built_y + SINK_DEPTH)
        {
            _start_falling();
            return;
        }
    }
}

void BridgeSystem::_run_off_end()
{
    // The engine goes over the end of the rails and the rest of the train
    // follows it, one vehicle at a time.
    const int last = _path.size() - 1;
    const bn::fixed total = _path_cum[last];
    const bn::fixed_point start = _nodes[START_NODE].position;
    const bn::fixed speed = _current_train_speed();
    _train_distance += speed;

    for(int axle = 0; axle < axle_count(); ++axle)
    {
        const bn::fixed distance = _axle_distance(axle);
        bn::fixed_point& at = _axle_position[axle];

        if(distance >= total)
        {
            _axle_fall[axle] += GRAVITY;
            at.set_x(at.x() + speed / 2);
            at.set_y(at.y() + _axle_fall[axle]);

            if(at.y() > FALL_Y_LIMIT)
            {
                at.set_y(FALL_Y_LIMIT);
            }
        }
        else if(distance <= 0)
        {
            at = bn::fixed_point(start.x() + distance, start.y());
        }
        else
        {
            const int segment = _segment_at(distance);
            const bn::fixed span = _path_cum[segment + 1] - _path_cum[segment];
            at = physics::lerp(_nodes[_path[segment]].position, _nodes[_path[segment + 1]].position,
                               (distance - _path_cum[segment]).safe_division(span));
        }
    }
}

SimStatus BridgeSystem::update()
{
    constexpr int SETTLE_DAMPING_SHIFT = 4;
    constexpr int RUN_DAMPING_SHIFT = 5;

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

        if(_ran_off)
        {
            _run_off_end();
        }
        else
        {
            // The road gave way: the whole train goes down together.
            for(bn::fixed_point& axle : _axle_position)
            {
                axle.set_y(axle.y() + _fall_vy);

                if(axle.y() > FALL_Y_LIMIT)
                {
                    axle.set_y(FALL_Y_LIMIT);
                }
            }
        }

        // The wreck keeps falling without the train on it.
        for(int i = 0; i < _nodes.size(); ++i)
        {
            _load[i] = 0;
        }

        for(int step = 0; step < SIM_SUBSTEPS; ++step)
        {
            _substep(256, RUN_DAMPING_SHIFT);
        }

        return _status;
    }

    if(_status != SimStatus::running)
    {
        return _status;
    }

    ++_test_frames;

    int gravity_q8 = 256;
    int damping_shift = RUN_DAMPING_SHIFT;

    if(_settle_frames > 0)
    {
        --_settle_frames;
        damping_shift = SETTLE_DAMPING_SHIFT;

        if(_test_frames < GRAVITY_RAMP_FRAMES)
        {
            gravity_q8 = (_test_frames * 256) / GRAVITY_RAMP_FRAMES;
        }
    }
    else
    {
        _train_distance += _current_train_speed().safe_multiplication(_grade_factor());
        ++_accel_frames;
    }

    _apply_train_load();

    for(int step = 0; step < SIM_SUBSTEPS; ++step)
    {
        _substep(gravity_q8, damping_shift);
    }

    const int stress = stress_percent();

    if(stress > _peak_stress_percent)
    {
        _peak_stress_percent = stress;
    }

    _place_train();
    return _status;
}

} // namespace bridge
