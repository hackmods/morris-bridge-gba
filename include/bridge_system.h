#ifndef MORRIS_BRIDGE_BRIDGE_SYSTEM_H
#define MORRIS_BRIDGE_BRIDGE_SYSTEM_H

/**
 * @file
 * Node/strut truss, structural simulation, and train traversal.
 *
 * Test mode runs a pin-jointed spring truss: every joint has weight, every
 * link is an axial spring that snaps past its material strength, anchors are
 * fixed, and the train is a moving set of axle loads shared between the two
 * joints of the link it stands on. Nothing is rated up front — a link holds
 * or breaks because of the force that actually reaches it.
 *
 * Storage is `bn::vector` with compile-time capacities (16 nodes, 32 struts).
 * Nothing here calls `new`/`delete` or the heap, and there is no float.
 */

#include <stdint.h>

#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_vector.h"

#include "physics.h"
#include "engine.h"
#include "level.h"

namespace bridge
{

inline constexpr int MAX_NODES = 16;
inline constexpr int MAX_STRUTS = 32;
inline constexpr int LEVEL_COUNT = level::COUNT;

/// Indices of the seeded start (left bank) and end (right bank) nodes.
inline constexpr int START_NODE = 0;
inline constexpr int END_NODE = 1;

inline constexpr bn::fixed ENGINE_MIN_SPEED = bn::fixed(1) / 4;
inline constexpr int ACCEL_FRAMES = 24;              // frames to reach cruise from rest
inline constexpr bn::fixed GRAVITY = bn::fixed(1) / 4;
inline constexpr bn::fixed FALL_INITIAL_VY = bn::fixed(1) / 2;
inline constexpr bn::fixed SNAP_RADIUS = 10;
inline constexpr bn::fixed FALL_Y_LIMIT = 90;

/// Links shorter than this are rejected (joints snap together well before it).
inline constexpr bn::fixed MIN_LINK_LENGTH = 8;

/// How close the cursor must be to a link to pick it for deletion.
inline constexpr bn::fixed LINK_PICK_RADIUS = 5;

/// Structure settles under its own weight for this long before the train moves.
inline constexpr int SETTLE_FRAMES = 40;
/// Self-weight fades in over this many frames so a new span does not ring.
inline constexpr int GRAVITY_RAMP_FRAMES = 24;
/// Solver substeps per frame (gravity is 1/64 px per substep², 1/4 px per frame²).
inline constexpr int SIM_SUBSTEPS = 4;

/// A train that sags this far below the rail line is in the water.
inline constexpr bn::fixed SINK_DEPTH = 20;

/// Track steeper than 1-in-`MAX_GRADE_RUN` is not a route — engines stall on it.
inline constexpr int MAX_GRADE_RUN = 2;

/// A Troublesome Truck's load swings this far either side of its weight, and
/// flips every `TRUCK_JOLT_FRAMES` as the trucks bump each other along.
inline constexpr bn::fixed TRUCK_JOLT = bn::fixed(1) / 4;
inline constexpr int TRUCK_JOLT_FRAMES = 8;

/// Weight of one bare joint (pin, plates) in engine-weight units.
inline constexpr bn::fixed JOINT_WEIGHT = bn::fixed(1) / 4;

enum class LinkRole : uint8_t
{
    track,  ///< Routable timber deck — train follows these links.
    brace,  ///< Structural rod — supports joints, not traversable.
    cable,  ///< Wire rope — holds in tension only, goes slack if pushed.
};

inline constexpr int LINK_ROLE_COUNT = 3;

enum class MaterialId : uint8_t
{
    timber,  ///< Wood beam — cheap, short, carries the rails.
    rod,     ///< Steel rod — dearer, longer, stronger, not a train route.
    cable,   ///< Wire rope — cheap, longest, strong in tension, useless in compression.
};

/**
 * @brief Structural properties of one material.
 *
 * Forces are in engine-weight units (Thomas weighs 4). `stiffness` is the
 * axial rigidity EA: force = stiffness × strain.
 */
struct Material
{
    bn::fixed stiffness;
    bn::fixed strength;       ///< Axial force (tension or compression) that snaps it.
    bn::fixed weight_per_px;
    int cost;
    int reach_num;            ///< Max length = level beam length × num / den.
    int reach_den;
    bool tension_only;        ///< Carries no force when shorter than its rest length.
};

inline constexpr Material TIMBER = { 480, 8, bn::fixed(1) / 128, 1, 1, 1, false };
inline constexpr Material ROD = { 720, 12, bn::fixed(1) / 128, 2, 3, 2, false };
inline constexpr Material CABLE = { 720, 12, bn::fixed(1) / 256, 1, 2, 1, true };

inline constexpr int NODE_COST = 1;
inline constexpr int TIMBER_LINK_COST = TIMBER.cost;
inline constexpr int ROD_LINK_COST = ROD.cost;

[[nodiscard]] inline constexpr MaterialId material_for_role(const LinkRole role)
{
    return role == LinkRole::brace ? MaterialId::rod
                                   : (role == LinkRole::cable ? MaterialId::cable : MaterialId::timber);
}

[[nodiscard]] inline constexpr const Material& material(const MaterialId id)
{
    return id == MaterialId::rod ? ROD : (id == MaterialId::cable ? CABLE : TIMBER);
}

[[nodiscard]] inline constexpr int link_cost(const MaterialId id)
{
    return material(id).cost;
}

[[nodiscard]] inline constexpr uint8_t link_role_bit(const LinkRole role)
{
    return uint8_t(1) << int(role);
}

enum class SimStatus
{
    idle,        ///< Build mode; simulation is not running.
    running,     ///< Structure is loaded and the train is settling or crossing.
    success,     ///< The whole train is on the far bank.
    failed_fall, ///< The road under the train snapped, vanished, or sank.
    no_path      ///< No climbable track joins the two banks.
};

enum class StrutPreviewStatus
{
    ok,
    same_node,
    invalid_node,
    duplicate,
    struts_full,
    zero_length,
    too_long,
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
    bn::fixed max_length = 0;
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
    MaterialId material = MaterialId::timber;
    bn::fixed length = 0;           ///< Rest length (as built).
    bn::fixed current_tension = 0;  ///< |axial force| this frame.
    bn::fixed max_tension = 0;      ///< Material strength.
    bn::fixed peak_tension = 0;     ///< Worst |axial force| of the last test run.
    bool compressed = false;        ///< Current force is compression, not tension.

    // Solver cache, filled by begin_test().
    int32_t inv_rest2 = 0;          ///< 2^56 / rest² (rest in bn::fixed data units).
    int32_t inv_rest = 0;           ///< 2^32 / rest.
    int32_t ea_q4 = 0;              ///< Stiffness × 16.
    bool tension_only = false;      ///< Cable: no force in compression.
};

/// How hard a link is working, for colouring: calm, warm (half its strength)
/// or hot (four-fifths, close to snapping).
enum class Heat : uint8_t
{
    calm,
    warm,
    hot,
};

[[nodiscard]] inline Heat heat_for(const bn::fixed force, const bn::fixed strength)
{
    if(force * 5 >= strength * 4)
    {
        return Heat::hot;
    }

    return force * 2 >= strength ? Heat::warm : Heat::calm;
}

/// Heat from the force a link carries right now (test mode).
[[nodiscard]] inline Heat strut_heat(const Strut& strut)
{
    return heat_for(strut.current_tension, strut.max_tension);
}

/// Heat from the worst force of the last test run (build mode); a link that
/// snapped reads as hot.
[[nodiscard]] inline Heat strut_peak_heat(const Strut& strut)
{
    return heat_for(strut.peak_tension, strut.max_tension);
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

    /// The bridge as welded: unlike struts(), links snapped mid-test still count.
    [[nodiscard]] const bn::vector<Strut, MAX_STRUTS>& built_struts() const
    {
        return _test_ran ? _saved_struts : _struts;
    }

    /**
     * @brief Place a node. Returns the new index, or empty if the vector is full.
     */
    [[nodiscard]] bn::optional<int> add_node(bn::fixed x, bn::fixed y, bool is_anchored = false);

    /**
     * @brief Connect two nodes with a link of the role's material.
     *
     * Rejects identical endpoints, missing IDs, duplicates, a full vector, and
     * links shorter than `MIN_LINK_LENGTH` or longer than the material reach.
     */
    [[nodiscard]] bn::optional<int> add_strut(int node_a, int node_b,
                                              LinkRole role = LinkRole::track);

    [[nodiscard]] bool allows_link_role(LinkRole role) const;

    /// Longest link of this role the level's yard can supply.
    [[nodiscard]] bn::fixed max_link_length(LinkRole role) const;

    /**
     * @brief Nearest node within `SNAP_RADIUS`, using squared distance.
     */
    [[nodiscard]] bn::optional<int> find_node_near(bn::fixed_point point) const;

    [[nodiscard]] bn::optional<int> find_strut(int node_a, int node_b) const;

    /// Link passing within `LINK_PICK_RADIUS` of @p point (nearest wins).
    [[nodiscard]] bn::optional<int> find_strut_near(bn::fixed_point point) const;

    /// Remove one link in build mode. False while testing or for a bad index.
    [[nodiscard]] bool remove_strut(int strut_index);

    /// Worst load on any standing link right now, as a percentage of its strength.
    [[nodiscard]] int stress_percent() const;

    /// Worst `stress_percent()` seen so far in this test run.
    [[nodiscard]] int peak_stress_percent() const
    {
        return _peak_stress_percent;
    }

    [[nodiscard]] StrutPreview preview_strut(int node_a, int node_b,
                                               LinkRole role = LinkRole::track) const;

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

    /// True when this link is track an engine can climb (grade within limit).
    [[nodiscard]] bool is_routable(const Strut& strut) const;

    [[nodiscard]] bool has_track_path() const;

    /**
     * @brief Begin test mode: snapshot the build, pick the shortest climbable
     * track route, weigh the structure, and park the train on the start bank.
     */
    SimStatus begin_test();

    /**
     * @brief Leave test mode and restore the build to its pre-test snapshot.
     *
     * Each restored link keeps the worst force it saw as `peak_tension`.
     */
    void end_test();

    /**
     * @brief Advance the structure and the train one frame.
     */
    SimStatus update();

    [[nodiscard]] SimStatus status() const
    {
        return _status;
    }

    [[nodiscard]] bn::fixed_point train_position() const
    {
        return _axle_position[0];
    }

    /// Engine (0) plus one axle per car behind it.
    [[nodiscard]] int axle_count() const
    {
        return 1 + _cars.size();
    }

    /// What the car on axle @p axle (1..) is.
    [[nodiscard]] engine::CarKind car_kind(const int axle) const
    {
        return _cars[axle - 1].kind;
    }

    /// Rail position of the engine (0) or a car (1..), on the bridge or a bank.
    [[nodiscard]] bn::fixed_point axle_position(const int axle) const
    {
        return _axle_position[axle];
    }

    [[nodiscard]] bool train_falling() const
    {
        return _falling;
    }

    /// True while the structure is still settling and the train has not left.
    [[nodiscard]] bool settling() const
    {
        return _status == SimStatus::running && _settle_frames > 0;
    }

    /// Links snapped so far in this test run.
    [[nodiscard]] int snapped_count() const
    {
        return _snapped_count;
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

    /// Engine, cars and any banker — the full load the bridge must carry.
    [[nodiscard]] bn::fixed train_total_weight() const;

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

    [[nodiscard]] const level::Def& level_def() const
    {
        return level::get(_level_index);
    }

    [[nodiscard]] const SnapFlash* snap_flash() const;

private:
    enum class LastEdit
    {
        none,
        node,
        strut,
    };

    /// Per-node solver state; only meaningful between begin_test() and end_test().
    struct NodeSim
    {
        int32_t vx = 0;           ///< Velocity, px × 2^20 per substep.
        int32_t vy = 0;
        int32_t fx = 0;           ///< Force accumulator, bn::fixed data.
        int32_t fy = 0;
        int32_t weight = 0;       ///< Dead weight (joint + half of each link).
        int32_t inv_inertia = 0;  ///< 1 / inertial mass, bn::fixed data.
    };

    void _seed_level();
    void _remove_strut_at(int strut_index);
    void _record_node_edit(int node_id);
    void _record_strut_edit(int strut_index);
    bool _find_route();
    void _prepare_solver();
    void _refresh_inertia();
    void _apply_axle(bn::fixed distance, bn::fixed weight);
    void _apply_train_load();
    void _substep(int gravity_q8, int damping_shift);
    void _break_strut(int strut_index);
    void _start_falling();
    void _place_train();
    [[nodiscard]] bn::fixed _axle_distance(int axle) const;
    [[nodiscard]] int _segment_at(bn::fixed distance) const;
    void _park_train();
    void _rebuild_train();
    [[nodiscard]] bn::fixed _axle_weight(int axle) const;
    [[nodiscard]] bn::fixed _grade_factor() const;
    bn::fixed _current_train_speed() const;

    bn::vector<Node, MAX_NODES> _nodes;
    bn::vector<Strut, MAX_STRUTS> _struts;
    bn::vector<Node, MAX_NODES> _saved_nodes;
    bn::vector<Strut, MAX_STRUTS> _saved_struts;
    bn::vector<int, MAX_NODES> _path;
    bn::fixed _path_cum[MAX_NODES] = {};  ///< Rest distance from start to each path node.
    NodeSim _sim[MAX_NODES] = {};
    int32_t _load[MAX_NODES] = {};        ///< Train load per node this frame, bn::fixed data.

    SimStatus _status = SimStatus::idle;
    bn::fixed_point _axle_position[1 + engine::MAX_CARS];
    bn::fixed _train_distance = 0;  ///< Engine distance along the route.
    bn::fixed _train_t = 0;         ///< 0..1 along the current strut.
    int _path_index = 0;            ///< Index of the node the train is leaving.
    int _accel_frames = 0;
    int _settle_frames = 0;
    int _test_frames = 0;
    int _snapped_count = 0;
    int _peak_stress_percent = 0;
    bn::fixed _fall_vy = 0;
    bool _falling = false;
    bool _test_ran = false;
    int _level_index = 0;
    int _anchored_node_count = 0;
    engine::Id _engine = engine::default_engine();
    bn::fixed _train_weight = 0;
    bn::fixed _train_speed = 1;
    bn::fixed _beam_length = level::DEFAULT_BEAM_LENGTH;
    bn::vector<engine::Car, engine::MAX_CARS> _cars;
    LastEdit _last_edit = LastEdit::none;
    int _last_edit_index = -1;
    bn::optional<SnapFlash> _snap_flash;
};

} // namespace bridge

#endif
