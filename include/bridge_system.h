#ifndef MORRIS_BRIDGE_BRIDGE_SYSTEM_H
#define MORRIS_BRIDGE_BRIDGE_SYSTEM_H

/**
 * @file
 * Node/strut graph and train traversal for Morris Bridge.
 *
 * Storage is `bn::vector` with compile-time capacities (16 nodes, 32 struts).
 * Nothing here calls `new`/`delete` or the heap.
 */

#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_vector.h"

#include "physics.h"
#include "engine.h"
#include "level.h"
#include "material.h"

namespace bridge
{

inline constexpr int MAX_NODES = 16;
inline constexpr int MAX_STRUTS = 32;
inline constexpr int LEVEL_COUNT = level::COUNT;

/// Indices of the seeded start (left bank) and end (right bank) nodes.
inline constexpr int START_NODE = 0;
inline constexpr int END_NODE = 1;

inline constexpr bn::fixed TRAIN_SPEED = 1;          // pixels per frame at cruise
inline constexpr bn::fixed ENGINE_MIN_SPEED = bn::fixed(1) / 4;
inline constexpr int ACCEL_FRAMES = 24;              // frames to reach cruise from rest
inline constexpr bn::fixed GRAVITY = bn::fixed(1) / 4;
inline constexpr bn::fixed FALL_INITIAL_VY = bn::fixed(1) / 2;
inline constexpr bn::fixed SNAP_RADIUS = 10;
inline constexpr bn::fixed FALL_Y_LIMIT = 90;

enum class LinkRole : uint8_t
{
    track,       ///< Routable deck — train follows these links.
    brace,       ///< Structural support — not traversable.
    foundation,  ///< Stone pier / ground anchor member.
};

using MaterialId = material::Id;

/// Joint plate cost in £ (vision budget uses member costs primarily).
inline constexpr int NODE_COST = 10;
inline constexpr int TIMBER_LINK_COST = 10; ///< wood
inline constexpr int ROD_LINK_COST = 25;    ///< iron
inline constexpr int STONE_LINK_COST = 50;
inline constexpr int CABLE_LINK_COST = 15;
inline constexpr bn::fixed ROD_STRENGTH_MUL = bn::fixed(5) / 4;

[[nodiscard]] inline constexpr MaterialId material_for_role(const LinkRole role)
{
    switch(role)
    {
        case LinkRole::foundation:
            return MaterialId::stone;
        case LinkRole::brace:
            return MaterialId::iron;
        default:
            return MaterialId::wood;
    }
}

[[nodiscard]] inline constexpr LinkRole role_for_material(const MaterialId material)
{
    switch(material)
    {
        case MaterialId::stone:
            return LinkRole::foundation;
        case MaterialId::cable:
        case MaterialId::iron:
            return LinkRole::brace;
        default:
            return LinkRole::track;
    }
}

[[nodiscard]] inline constexpr int link_cost(const MaterialId mat)
{
    return material::cost(mat);
}

[[nodiscard]] inline constexpr bn::fixed material_strength(const MaterialId mat)
{
    return material::get(mat).stiffness_mul;
}

[[nodiscard]] inline constexpr uint8_t link_role_bit(const LinkRole role)
{
    return uint8_t(1) << int(role);
}

[[nodiscard]] inline constexpr bool is_routable(const LinkRole role)
{
    return role == LinkRole::track;
}

[[nodiscard]] inline constexpr uint8_t link_material_bit(const MaterialId mat)
{
    return uint8_t(1) << int(mat);
}

enum class SimStatus
{
    idle,        ///< EDIT_MODE; simulation is not running.
    running,     ///< SIMULATION_MODE; train is traversing struts.
    success,     ///< RESULT_MODE success — reached the right bank.
    failed_fall, ///< RESULT_MODE failure — snapped / DERAILED.
    no_path      ///< Start and end are not connected.
};

/// Train state during / after simulation (vision: DERAILED on collapse).
enum class TrainState : uint8_t
{
    waiting,   ///< EDIT_MODE — parked on left bank.
    crossing,  ///< SIMULATION_MODE — on track.
    arrived,   ///< RESULT success.
    derailed,  ///< RESULT failure — bridge collapse / fall.
};

enum class StrutPreviewStatus
{
    ok,
    same_node,
    invalid_node,
    duplicate,
    struts_full,
    zero_length,
};

struct SnapFlash
{
    bn::fixed_point a;
    bn::fixed_point b;
    int frames = 0;
};

struct StrutPreview
{
    StrutPreviewStatus status = StrutPreviewStatus::invalid_node;
    bn::fixed length = 0;
    bn::fixed max_tension = 0;
};

enum class UndoResult
{
    nothing,
    removed_strut,
    removed_node,
};

enum class RemoveNodeResult
{
    ok,
    invalid,
    anchored,
    in_test,
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
    LinkRole role = LinkRole::track;
    MaterialId material = MaterialId::wood;
    bn::fixed length = 0;
    bn::fixed current_tension = 0; ///< Axial |F| from the truss load-path solve.
    bn::fixed max_tension = 0;     ///< Rated yield (collapse when stress >= 1).
    bn::fixed stress = 0;          ///< current_tension / max_tension.
    bool triangulated = false;
};

[[nodiscard]] inline bool strut_warns_for_load(const Strut& strut, const bn::fixed load)
{
    return strut.max_tension < load;
}

class BridgeSystem
{
public:
    BridgeSystem();

    void reset_level();
    void load_level(int level_index);

    [[nodiscard]] int current_level() const
    {
        return _level_index;
    }

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
    [[nodiscard]] bn::optional<int> add_strut(int node_a, int node_b,
                                              LinkRole role = LinkRole::track,
                                              MaterialId material = MaterialId::wood);

    [[nodiscard]] bool allows_link_role(LinkRole role) const;

    [[nodiscard]] bool allows_material(MaterialId material) const;

    /**
     * @brief Nearest node within `SNAP_RADIUS`, using squared distance.
     */
    [[nodiscard]] bn::optional<int> find_node_near(bn::fixed_point point) const;

    /**
     * @brief Nearest strut whose segment is within `SNAP_RADIUS` of `point`.
     */
    [[nodiscard]] bn::optional<int> find_strut_near(bn::fixed_point point) const;

    [[nodiscard]] bn::optional<int> find_strut(int node_a, int node_b) const;

    [[nodiscard]] StrutPreview preview_strut(int node_a, int node_b,
                                               LinkRole role = LinkRole::track,
                                               MaterialId material = MaterialId::wood) const;

    /// Capacity used by the weld ghost when the cursor is not over a joint.
    [[nodiscard]] bn::fixed span_preview_capacity(bn::fixed length, LinkRole role,
                                                    MaterialId material = MaterialId::wood) const;

    /**
     * @brief Delete a player-built strut by index (EDIT_MODE B: DELETE).
     */
    [[nodiscard]] bool remove_strut(int strut_index);

    /**
     * @brief Remaining £ budget (level purse minus spent members / joints).
     */
    [[nodiscard]] int budget_remaining() const;

    [[nodiscard]] int budget_spent() const;

    [[nodiscard]] TrainState train_state() const
    {
        return _train_state;
    }

    /**
     * @brief Peak stress ratio across live members (HUD STRESS % during SIM).
     */
    [[nodiscard]] bn::fixed peak_stress() const;

    [[nodiscard]] UndoResult undo_last_edit();

    [[nodiscard]] RemoveNodeResult remove_player_node(int node_id);

    [[nodiscard]] int anchored_node_count() const
    {
        return _anchored_node_count;
    }

    [[nodiscard]] int player_node_count() const
    {
        return _nodes.size() - _anchored_node_count;
    }

    [[nodiscard]] int player_node_budget() const
    {
        const int budget = MAX_NODES - _anchored_node_count;
        return budget > 0 ? budget : 0;
    }

    [[nodiscard]] bool is_player_node(int node_id) const;

    [[nodiscard]] bool has_track_path() const;

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

    void set_engine(engine::Id id);

    [[nodiscard]] engine::Id engine() const
    {
        return _engine;
    }

    [[nodiscard]] bn::fixed train_weight() const
    {
        return _train_weight;
    }

    [[nodiscard]] bn::fixed train_link_t() const
    {
        return _train_t;
    }

    [[nodiscard]] int train_path_index() const
    {
        return _path_index;
    }

    [[nodiscard]] bn::fixed current_train_speed() const;

    [[nodiscard]] bn::fixed train_speed() const
    {
        return _train_speed;
    }

    [[nodiscard]] bn::fixed safe_length() const
    {
        return _safe_length;
    }

    [[nodiscard]] int coach_count() const
    {
        return _coach_count;
    }

    [[nodiscard]] const level::Def& level_def() const
    {
        return level::get(_level_index);
    }

    [[nodiscard]] bn::fixed effective_weight_at(bn::fixed distance_along, bn::fixed strut_length) const;

    [[nodiscard]] const SnapFlash* snap_flash() const;

private:
    enum class LastEdit
    {
        none,
        node,
        strut,
    };

    void _seed_level();
    void _recompute_strut_ratings();
    int _connection_count(int node_id) const;
    bool _strut_is_triangulated(int strut_index) const;
    bn::fixed _current_train_load() const;
    bn::fixed _current_train_speed() const;
    void _break_strut(int strut_index);
    void _clear_member_forces();
    void _solve_truss_stress(int active_strut, bn::fixed distance_along);
    bool _cascade_collapse(int active_strut);
    void _apply_tension(int strut_index, bn::fixed distance_along);
    void _remove_strut_at(int strut_index);
    void _record_node_edit(int node_id);
    void _record_strut_edit(int strut_index);
    bool _bfs_path();
    void _start_falling();
    [[nodiscard]] bool _pier_placement_ok(int node_a, int node_b) const;

    bn::vector<Node, MAX_NODES> _nodes;
    bn::vector<Strut, MAX_STRUTS> _struts;
    bn::vector<Strut, MAX_STRUTS> _saved_struts;
    bn::vector<int, MAX_NODES> _path;

    SimStatus _status = SimStatus::idle;
    TrainState _train_state = TrainState::waiting;
    bn::fixed_point _train_position;
    bn::fixed _train_t = 0;     ///< 0..1 along the current strut.
    int _path_index = 0;        ///< Index of the node the train is leaving.
    int _accel_frames = 0;
    bn::fixed _fall_vy = 0;
    bool _falling = false;
    int _level_index = 0;
    int _anchored_node_count = 0;
    engine::Id _engine = engine::default_engine();
    bn::fixed _train_weight = physics::BASE_WEIGHT;
    bn::fixed _train_speed = TRAIN_SPEED;
    bn::fixed _safe_length = physics::SAFE_LENGTH;
    int _coach_count = 0;
    LastEdit _last_edit = LastEdit::none;
    int _last_edit_index = -1;
    bn::optional<SnapFlash> _snap_flash;
};

} // namespace bridge

#endif
