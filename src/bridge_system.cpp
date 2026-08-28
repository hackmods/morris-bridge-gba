#include "bridge_system.h"

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
    _fall_vy = 0;
    _falling = false;
    _last_edit = LastEdit::none;
    _last_edit_index = -1;
    _seed_canyon();
    _train_position = _nodes[START_NODE].position;
}

void BridgeSystem::_seed_canyon()
{
    _anchored_node_count = 0;

    // Left bank (start) then right bank (end). Additional anchors give Leo
    // places to pin the deck without spending his node budget.
    // Butano screen space is centred: x in [-120, 120), y in [-80, 80).
    (void)add_node(-88, 16, true); // START_NODE
    (void)add_node(88, 16, true);  // END_NODE
    (void)add_node(-104, 40, true);
    (void)add_node(-88, 40, true);
    (void)add_node(-72, 40, true);
    (void)add_node(72, 40, true);
    (void)add_node(88, 40, true);
    (void)add_node(104, 40, true);
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

bn::optional<int> BridgeSystem::add_strut(int node_a, int node_b)
{
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
    strut.length = length;
    strut.current_tension = 0;
    strut.max_tension = physics::max_tension_for_length(length);
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

StrutPreview BridgeSystem::preview_strut(int node_a, int node_b) const
{
    StrutPreview preview;

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
    preview.max_tension = physics::max_tension_for_length(length);
    return preview;
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

            const RemoveNodeResult removed = remove_player_node(_last_edit_index);

            if(removed == RemoveNodeResult::ok)
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

    // Walk parent pointers into a scratch, then reverse into `_path`.
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
    _falling = false;
    _fall_vy = 0;
    _train_t = 0;
    _path_index = 0;
    _train_position = _nodes[START_NODE].position;

    for(int i = 0; i < _struts.size(); ++i)
    {
        _struts[i].current_tension = 0;
    }

    if(!_bfs_path())
    {
        _status = SimStatus::no_path;
        return _status;
    }

    _status = SimStatus::running;
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
    _falling = false;
    _fall_vy = 0;
    _train_t = 0;
    _path_index = 0;
    _train_position = _nodes[START_NODE].position;

    for(int i = 0; i < _struts.size(); ++i)
    {
        _struts[i].current_tension = 0;
    }
}

void BridgeSystem::_break_strut(int strut_index)
{
    _struts.erase(_struts.begin() + strut_index);
}

void BridgeSystem::_start_falling()
{
    _falling = true;
    _fall_vy = 0;
    _status = SimStatus::failed_fall;
}

void BridgeSystem::_apply_tension(int strut_index, bn::fixed distance_along)
{
    Strut& strut = _struts[strut_index];
    strut.current_tension = physics::tension(physics::BASE_WEIGHT, distance_along, strut.length);
}

SimStatus BridgeSystem::update()
{
    if(_status == SimStatus::failed_fall)
    {
        _fall_vy += GRAVITY;
        _train_position.set_y(_train_position.y() + _fall_vy);
        return _status;
    }

    if(_status != SimStatus::running)
    {
        return _status;
    }

    if(_falling)
    {
        _fall_vy += GRAVITY;
        _train_position.set_y(_train_position.y() + _fall_vy);

        if(_train_position.y() > FALL_Y_LIMIT)
        {
            _status = SimStatus::failed_fall;
        }

        return _status;
    }

    if(_path_index + 1 >= _path.size())
    {
        _status = SimStatus::success;
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
    const bn::fixed dt = length > 0 ? TRAIN_SPEED.safe_division(length) : bn::fixed(1);

    _train_t += dt;

    bn::fixed along = _train_t.safe_multiplication(length);

    if(along > length)
    {
        along = length;
    }

    _apply_tension(strut_index, along);

    if(physics::exceeds_max_tension(_struts[strut_index].current_tension,
                                    _struts[strut_index].max_tension))
    {
        _break_strut(strut_index);
        _start_falling();
        return _status;
    }

    if(_train_t >= 1)
    {
        _train_t = 0;
        _path_index += 1;
        _train_position = b.position;
        _struts[strut_index].current_tension = 0;

        if(_path_index + 1 >= _path.size())
        {
            _status = SimStatus::success;
        }
    }
    else
    {
        _train_position = physics::lerp(a.position, b.position, _train_t);
    }

    return _status;
}

} // namespace bridge
