#ifndef RYANS_BRIDGE_BRIDGE_SYSTEM_H
#define RYANS_BRIDGE_BRIDGE_SYSTEM_H

/**
 * @file
 * Node/strut graph and train traversal for Ryan's Bridge.
 *
 * Storage is `bn::vector` with compile-time capacities (16 nodes, 32 struts).
 * Nothing here calls `new`/`delete` or the heap.
 */

#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_vector.h"

#include "physics.h"

namespace bridge
{

inline constexpr int MAX_NODES = 16;
inline constexpr int MAX_STRUTS = 32;

/// Indices of the seeded start (left bank) and end (right bank) nodes.
inline constexpr int START_NODE = 0;
inline constexpr int END_NODE = 1;

inline constexpr bn::fixed TRAIN_SPEED = 1;          // pixels per frame
inline constexpr bn::fixed GRAVITY = bn::fixed(1) / 8;
inline constexpr bn::fixed SNAP_RADIUS = 6;
inline constexpr bn::fixed FALL_Y_LIMIT = 90;        // off the bottom of the 160px screen

enum class SimStatus
{
    idle,        ///< Build mode; simulation is not running.
    running,     ///< Train is traversing struts.
    success,     ///< Reached the end node.
    failed_fall, ///< Strut snapped or train walked off a missing edge.
    no_path      ///< Start and end are not connected.
};

struct Node
{
    bn::fixed_point position;
    bool is_anchored = false;
};

struct Strut
{
    int node_a = 0;
    int node_b = 0;
    bn::fixed length = 0;
    bn::fixed current_tension = 0;
    bn::fixed max_tension = 0;
};

class BridgeSystem
{
public:
    BridgeSystem();

    void reset_level();

    [[nodiscard]] const bn::vector<Node, MAX_NODES>& nodes() const
    {
        return _nodes;
    }

    [[nodiscard]] const bn::vector<Strut, MAX_STRUTS>& struts() const
    {
        return _struts;
    }

    [[nodiscard]] bn::vector<Node, MAX_NODES>& nodes()
    {
        return _nodes;
    }

    [[nodiscard]] bn::vector<Strut, MAX_STRUTS>& struts()
    {
        return _struts;
    }

    /**
     * @brief Place a node. Returns the new index, or empty if the vector is full.
     */
    [[nodiscard]] bn::optional<int> add_node(bn::fixed x, bn::fixed y, bool is_anchored = false);

    /**
     * @brief Connect two nodes with a strut.
     *
     * Rejects identical endpoints, missing IDs, duplicates, and a full vector.
     * Length and `max_tension` are computed immediately.
     */
    [[nodiscard]] bn::optional<int> add_strut(int node_a, int node_b);

    /**
     * @brief Nearest node within `SNAP_RADIUS`, using squared distance.
     */
    [[nodiscard]] bn::optional<int> find_node_near(bn::fixed_point point) const;

    [[nodiscard]] bn::optional<int> find_strut(int node_a, int node_b) const;

    /**
     * @brief Begin test mode: snapshot struts, BFS a path, spawn the train.
     */
    SimStatus begin_test();

    /**
     * @brief Leave test mode and restore struts to their pre-test snapshot.
     */
    void end_test();

    /**
     * @brief Advance the train one frame. No-op unless status is `running`.
     */
    SimStatus update();

    [[nodiscard]] SimStatus status() const
    {
        return _status;
    }

    [[nodiscard]] bn::fixed_point train_position() const
    {
        return _train_position;
    }

    [[nodiscard]] bool train_falling() const
    {
        return _falling;
    }

    [[nodiscard]] int path_length() const
    {
        return _path.size();
    }

private:
    void _seed_canyon();
    void _break_strut(int strut_index);
    void _apply_tension(int strut_index, bn::fixed distance_along);
    bool _bfs_path();
    void _start_falling();

    bn::vector<Node, MAX_NODES> _nodes;
    bn::vector<Strut, MAX_STRUTS> _struts;
    bn::vector<Strut, MAX_STRUTS> _saved_struts;
    bn::vector<int, MAX_NODES> _path;

    SimStatus _status = SimStatus::idle;
    bn::fixed_point _train_position;
    bn::fixed _train_t = 0;     ///< 0..1 along the current strut.
    int _path_index = 0;        ///< Index of the node the train is leaving.
    bn::fixed _fall_vy = 0;
    bool _falling = false;
};

} // namespace bridge

#endif
