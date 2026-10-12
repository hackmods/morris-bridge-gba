/**
 * @file tests/test_physics.cpp
 *
 * Host unit tests for the fixed-point geometry helpers and the truss solver.
 *
 * Compiles with stock GCC/Clang (no devkitARM, no Butano). The `tests/host_bn`
 * directory mocks `bn::fixed` as 20.12 with the same int64_t shift/divide
 * pattern the engine uses, then links the real `src/core/physics.cpp` and
 * `src/core/bridge_system.cpp`.
 *
 * The structural tests are written as bridge-builder invariants (unsupported
 * decks fall, every par link carries load, heavier trains need more bridge)
 * rather than as checks of particular force numbers.
 *
 * Build and run:
 *   make -C tests
 */

#include <cassert>
#include <iostream>

#include "physics.h"
#include "bridge_system.h"
#include "engine.h"
#include "level_solutions.h"

namespace
{

const char* const LEVEL_NAMES[level::COUNT] = {
    "Knapford", "Gordon's Hill", "Culdee Fell", "Tidmouth",
    "Wellsworth", "Crovan's Gate", "Callan", "Vicarstown",
};

void expect_near(bn::fixed actual, bn::fixed expected, bn::fixed slack, const char* name)
{
    const bn::fixed delta = physics::abs(actual - expected);

    if(delta > slack)
    {
        std::cerr << "FAIL " << name
                  << " actual.data=" << actual.data()
                  << " expected.data=" << expected.data()
                  << " slack.data=" << slack.data() << '\n';
        assert(false && "fixed-point value out of tolerance");
    }

    std::cout << "  ok " << name
              << "  actual=" << actual.integer() << "+" << actual.data() % 4096
              << "/4096\n";
}

/// Run a started-or-not system to a verdict; returns frames simulated.
int run(bridge::BridgeSystem& system, int max_frames = 1200)
{
    if(system.status() != bridge::SimStatus::running)
    {
        (void)system.begin_test();
    }

    int frames = 0;

    while(system.status() == bridge::SimStatus::running && frames < max_frames)
    {
        system.update();
        ++frames;
    }

    return frames;
}

/// Reference design for @p level_index with one link left out (-1 = none).
bool build_reference_without(bridge::BridgeSystem& system, int level_index, engine::Id engine_id,
                             int skipped_link, bool reverse_links = false)
{
    const level_solutions::Design& design = level_solutions::reference(level_index);
    system.load_level(level_index);
    system.set_engine(engine_id);

    for(int i = 0; i < design.joint_count; ++i)
    {
        if(!system.add_node(design.joints[i].x, design.joints[i].y, false))
        {
            return false;
        }
    }

    for(int step = 0; step < design.link_count; ++step)
    {
        const int i = reverse_links ? design.link_count - 1 - step : step;

        if(i == skipped_link)
        {
            continue;
        }

        const level_solutions::Link& link = design.links[i];

        if(!system.add_strut(link.a, link.b, link.role))
        {
            return false;
        }
    }

    return true;
}

/// A bare deck: equal joints strung straight from bank to bank, nothing else.
bool build_bare_deck(bridge::BridgeSystem& system, int level_index)
{
    system.load_level(level_index);
    system.set_engine(system.level_def().scheduled_engine);

    const bn::fixed_point start = system.nodes()[bridge::START_NODE].position;
    const bn::fixed_point end = system.nodes()[bridge::END_NODE].position;
    const int span = (end.x() - start.x()).integer();
    const int beam = system.max_link_length(bridge::LinkRole::track).integer();
    const int panels = (span + beam - 1) / beam;
    int previous = bridge::START_NODE;

    for(int i = 1; i < panels; ++i)
    {
        const bn::optional<int> joint =
            system.add_node(start.x() + bn::fixed(span * i) / panels,
                            start.y() + (end.y() - start.y()) * i / panels, false);

        if(!joint || !system.add_strut(previous, joint.value()))
        {
            return false;
        }

        previous = joint.value();
    }

    return system.add_strut(previous, bridge::END_NODE).has_value();
}

} // namespace

int main()
{
    std::cout << "Thomas's Bridge host physics tests (bn::fixed mock is 20.12)\n";

    // --- Geometry helpers ---------------------------------------------------

    {
        const bn::fixed_point a(0, 0);
        const bn::fixed_point b(3, 4);
        expect_near(physics::squared_distance(a, b), 25, 0, "squared_distance(3,4)");
        expect_near(physics::distance(a, b), 5, bn::fixed(1) / 16, "distance(3,4) ~ 5");
    }

    {
        expect_near(physics::approx_sqrt(100), 10, bn::fixed(1) / 16, "approx_sqrt(100)");
        expect_near(physics::approx_sqrt(0), 0, 0, "approx_sqrt(0)");
        expect_near(physics::approx_sqrt(-3), 0, 0, "approx_sqrt(negative)");
    }

    {
        const bn::fixed_point mid = physics::lerp(bn::fixed_point(0, 0), bn::fixed_point(10, 0),
                                                  bn::fixed(1) / 2);
        expect_near(mid.x(), 5, bn::fixed(1) / 16, "lerp midpoint x");
        expect_near(mid.y(), 0, 0, "lerp midpoint y");
    }

    {
        // A slack cable can fold up to a fraction of a pixel. The segment
        // count the link renderer divides by must never reach zero (it did:
        // "32bit integer division by zero" on Gordon's Hill).
        assert(physics::segments_for(bn::fixed(1) / 4, 24) == 1);
        assert(physics::segments_for(0, 16) == 1);
        assert(physics::segments_for(24, 24) == 1);
        assert(physics::segments_for(25, 24) == 2);
        assert(physics::segments_for(72, 24) == 3);
        assert(physics::segments_for(16, 16) == 1);
        assert(physics::segments_for(48, 16) == 3);
        std::cout << "  ok link segment count is never zero\n";
    }

    // --- Build rules --------------------------------------------------------

    {
        bridge::BridgeSystem system;
        system.load_level(0);
        assert(system.anchored_node_count() == 3);
        assert(system.player_node_budget() == bridge::MAX_NODES - 3);
        assert(system.nodes()[bridge::START_NODE].position.x().integer() == -40);
        assert(system.nodes()[bridge::START_NODE].position.y().integer() == 12);
        assert(system.level_def().deck_y.integer() == 12);

        system.load_level(1);
        assert(system.anchored_node_count() == 4);
        assert(system.nodes()[bridge::END_NODE].position.x().integer() == 80);

        system.load_level(-1);
        assert(system.current_level() == 0);
        system.load_level(99);
        assert(system.current_level() == level::COUNT - 1);
        std::cout << "  ok per-level seeding and clamping\n";
    }

    {
        bridge::BridgeSystem system;
        system.load_level(0);

        assert(system.preview_strut(bridge::START_NODE, bridge::START_NODE).status ==
               bridge::StrutPreviewStatus::same_node);
        assert(system.preview_strut(bridge::START_NODE, 99).status ==
               bridge::StrutPreviewStatus::invalid_node);

        // The yard cannot supply an 80 px beam: the banks do not join directly.
        const bridge::StrutPreview bank_to_bank =
            system.preview_strut(bridge::START_NODE, bridge::END_NODE);
        assert(bank_to_bank.status == bridge::StrutPreviewStatus::too_long);
        assert(bank_to_bank.length > bank_to_bank.max_length);
        assert(!system.add_strut(bridge::START_NODE, bridge::END_NODE));

        const bn::optional<int> mid = system.add_node(0, 12, false);
        assert(mid.has_value());
        assert(system.preview_strut(bridge::START_NODE, mid.value()).status ==
               bridge::StrutPreviewStatus::ok);
        assert(system.add_strut(bridge::START_NODE, mid.value()));
        assert(system.preview_strut(mid.value(), bridge::START_NODE).status ==
               bridge::StrutPreviewStatus::duplicate);

        const bn::optional<int> stub = system.add_node(4, 12, false);
        assert(stub.has_value());
        assert(system.preview_strut(mid.value(), stub.value()).status ==
               bridge::StrutPreviewStatus::zero_length);

        // Knapford has no rods yet.
        assert(!system.allows_link_role(bridge::LinkRole::brace));
        assert(!system.add_strut(mid.value(), bridge::END_NODE, bridge::LinkRole::brace));
        std::cout << "  ok link preview: length limits, duplicates, locked tools\n";
    }

    {
        bridge::BridgeSystem system;
        system.load_level(1);
        assert(system.max_link_length(bridge::LinkRole::brace) >
               system.max_link_length(bridge::LinkRole::track));
        assert(bridge::material(bridge::MaterialId::rod).strength >
               bridge::material(bridge::MaterialId::timber).strength);
        assert(bridge::link_cost(bridge::MaterialId::rod) >
               bridge::link_cost(bridge::MaterialId::timber));
        std::cout << "  ok rods reach further, hold more, and cost more than timber\n";
    }

    {
        bridge::BridgeSystem system;
        system.load_level(0);
        const bn::optional<int> mid = system.add_node(0, 12, false);
        assert(mid.has_value());
        assert(system.player_node_count() == 1);
        assert(system.add_strut(bridge::START_NODE, mid.value()));
        assert(system.undo_last_edit() == bridge::UndoResult::removed_strut);
        assert(system.struts().empty());
        // Undo keeps unwinding: the joint that beam reached goes next.
        assert(system.undo_last_edit() == bridge::UndoResult::removed_node);
        assert(system.player_node_count() == 0);
        assert(system.undo_last_edit() == bridge::UndoResult::nothing);

        const bn::optional<int> solo = system.add_node(-8, 0, false);
        assert(solo.has_value());
        assert(system.undo_last_edit() == bridge::UndoResult::removed_node);

        // A whole par bridge unwinds to bare anchors, one press at a time.
        assert(level_solutions::build_reference(system, 2));
        int presses = 0;

        while(system.undo_last_edit() != bridge::UndoResult::nothing)
        {
            ++presses;
        }

        assert(presses == 5 + 11);
        assert(system.struts().empty() && system.player_node_count() == 0);
        system.load_level(0);
        assert(system.remove_player_node(bridge::START_NODE) == bridge::RemoveNodeResult::anchored);

        const int budget = system.player_node_budget() - system.player_node_count();

        for(int i = 0; i < budget; ++i)
        {
            assert(system.add_node(-40 + i * 4, -40, false).has_value());
        }

        assert(!system.add_node(0, -60, false).has_value());
        std::cout << "  ok undo, delete, and joint budget\n";
    }

    // --- Routing ------------------------------------------------------------

    {
        bridge::BridgeSystem system;
        system.load_level(0);
        assert(!system.has_track_path());
        assert(system.begin_test() == bridge::SimStatus::no_path);
        system.end_test();
        assert(level_solutions::build_reference(system, 0));
        assert(system.has_track_path());
        std::cout << "  ok a track route is required before a test run\n";
    }

    {
        bridge::BridgeSystem system;
        system.load_level(1);
        const bn::optional<int> a = system.add_node(-40, 36, false);
        const bn::optional<int> b = system.add_node(0, 36, false);
        const bn::optional<int> c = system.add_node(40, 36, false);
        assert(a && b && c);
        assert(system.add_strut(bridge::START_NODE, a.value(), bridge::LinkRole::cable));
        assert(system.add_strut(a.value(), b.value(), bridge::LinkRole::cable));
        assert(system.add_strut(b.value(), c.value(), bridge::LinkRole::cable));
        assert(system.add_strut(c.value(), bridge::END_NODE, bridge::LinkRole::cable));
        assert(system.begin_test() == bridge::SimStatus::no_path);
        std::cout << "  ok cables and rods are not a train route\n";
    }

    {
        // Track dropping 28 px over 16 px is far past the 2-in-3 limit.
        bridge::BridgeSystem system;
        system.load_level(0);
        const bn::optional<int> low = system.add_node(-24, 40, false);
        assert(low.has_value());
        assert(system.add_strut(bridge::START_NODE, low.value()));
        assert(system.add_strut(low.value(), 2));
        assert(!system.is_routable(system.struts()[0]));

        const bn::optional<int> level_joint = system.add_node(-8, 12, false);
        assert(level_joint.has_value());
        assert(system.add_strut(bridge::START_NODE, level_joint.value()));
        assert(system.is_routable(system.struts()[2]));
        std::cout << "  ok engines refuse track steeper than the grade limit\n";
    }

    {
        // Gordon's Hill with the whole climb left to the last beam: the deck
        // is complete, no engine can take it, and that beam is singled out.
        bridge::BridgeSystem system;
        system.load_level(1);
        const bn::optional<int> a = system.add_node(-40, 44, false);
        const bn::optional<int> b = system.add_node(0, 44, false);
        const bn::optional<int> c = system.add_node(44, 44, false);
        assert(a && b && c);
        assert(system.add_strut(bridge::START_NODE, a.value()));
        assert(system.add_strut(a.value(), b.value()));
        assert(system.add_strut(b.value(), c.value()));
        assert(!system.steep_strut());
        const bn::optional<int> ramp = system.add_strut(c.value(), bridge::END_NODE);
        assert(ramp.has_value());
        assert(!system.has_track_path());
        assert(system.has_track_path(true));
        assert(system.steep_strut().value() == ramp.value());
        // A cable up to the crag is steep too, but it is not what stops the train.
        assert(system.add_strut(2, a.value(), bridge::LinkRole::cable));
        assert(system.steep_strut().value() == ramp.value());

        // The same climb shared between two beams is a route.
        assert(system.remove_player_node(c.value()) == bridge::RemoveNodeResult::ok);
        const bn::optional<int> eased = system.add_node(40, 28, false);
        assert(eased.has_value());
        assert(system.add_strut(b.value(), eased.value()));
        assert(system.add_strut(eased.value(), bridge::END_NODE));
        assert(system.has_track_path());
        assert(!system.steep_strut());
        std::cout << "  ok a deck too steep to climb names the beam at fault\n";
    }

    {
        // Tidmouth's truss is all timber; the train must still take the deck,
        // not the top chord.
        bridge::BridgeSystem system;
        assert(level_solutions::build_reference(system, 3));
        assert(system.begin_test() == bridge::SimStatus::running);
        assert(system.path_length() == 4);
        std::cout << "  ok the route follows the deck, not the truss above it\n";
    }

    // --- Structural invariants ---------------------------------------------

    {
        // Gravity is real: a joint welded to nothing falls out of the sky.
        bridge::BridgeSystem system;
        assert(level_solutions::build_reference(system, 0));
        const bn::optional<int> loose = system.add_node(0, -40, false);
        assert(loose.has_value());
        assert(system.begin_test() == bridge::SimStatus::running);

        for(int i = 0; i < bridge::SETTLE_FRAMES; ++i)
        {
            system.update();
            assert(system.train_position().x() == system.nodes()[bridge::START_NODE].position.x());
        }

        assert(!system.settling());
        assert(system.nodes()[loose.value()].position.y() > bn::fixed(-30));
        std::cout << "  ok loose joints fall while the train waits for the bridge to settle\n";
    }

    for(int level_index = 0; level_index < level::COUNT; ++level_index)
    {
        bridge::BridgeSystem system;
        assert(build_bare_deck(system, level_index));
        assert(system.has_track_path());
        run(system);
        assert(system.status() == bridge::SimStatus::failed_fall);
        assert(system.snapped_count() > 0);
        std::cout << "  ok " << LEVEL_NAMES[level_index] << ": an unsupported deck collapses\n";
    }

    for(int level_index = 0; level_index < level::COUNT; ++level_index)
    {
        const level_solutions::Design& design = level_solutions::reference(level_index);
        const engine::Id scheduled = level::get(level_index).scheduled_engine;

        bridge::BridgeSystem system;
        assert(level_solutions::build_reference(system, level_index));
        const int frames = run(system);
        assert(system.status() == bridge::SimStatus::success);
        assert(system.snapped_count() == 0);

        // Weld order must not change the physics.
        bridge::BridgeSystem reversed;
        assert(build_reference_without(reversed, level_index, scheduled, -1, true));
        assert(run(reversed) == frames);
        assert(reversed.status() == bridge::SimStatus::success);

        // No dead weight at par: take any one link away and the train is lost.
        for(int i = 0; i < design.link_count; ++i)
        {
            bridge::BridgeSystem weakened;
            assert(build_reference_without(weakened, level_index, scheduled, i));
            run(weakened);
            assert(weakened.status() != bridge::SimStatus::success);
        }

        // The express needs more bridge than the timetabled engine does —
        // except on the hill, where Edward banking gets even Gordon up.
        bridge::BridgeSystem heavy;
        assert(build_reference_without(heavy, level_index, engine::Id::gordon, -1));
        run(heavy);
        const bool banked = level::get(level_index).banked;
        assert(heavy.status() ==
               (banked ? bridge::SimStatus::success : bridge::SimStatus::failed_fall));

        std::cout << "  ok " << LEVEL_NAMES[level_index] << ": par design holds in " << frames
                  << " frames, every link carries load, Gordon "
                  << (banked ? "is banked over it" : "breaks it") << "\n";
    }

    {
        // A link between two anchors is bolted to bedrock at both ends: it
        // costs money and carries nothing.
        bridge::BridgeSystem system;
        assert(level_solutions::build_reference(system, 4));
        const bn::optional<int> idle = system.add_strut(bridge::START_NODE, 2);
        assert(idle.has_value());
        run(system);
        assert(system.status() == bridge::SimStatus::success);
        assert(system.struts()[idle.value()].peak_tension == bn::fixed(0));
        std::cout << "  ok a link between two anchors carries no load\n";
    }

    {
        // Same Knapford bridge, lighter and heavier trains.
        bridge::BridgeSystem light;
        assert(build_reference_without(light, 0, engine::Id::percy, -1));
        run(light);
        assert(light.status() == bridge::SimStatus::success);
        const bn::fixed light_peak = light.struts()[2].peak_tension;

        bridge::BridgeSystem scheduled;
        assert(build_reference_without(scheduled, 0, engine::Id::thomas, -1));
        run(scheduled);
        assert(scheduled.status() == bridge::SimStatus::success);
        assert(scheduled.struts()[2].peak_tension > light_peak);
        assert(scheduled.struts()[2].compressed || scheduled.struts()[2].peak_tension > 0);

        // The post carries the engine plus its share of the deck, no more
        // than a comfortable margin above the engine's own weight.
        assert(scheduled.struts()[2].peak_tension > scheduled.train_weight());
        assert(scheduled.struts()[2].peak_tension < scheduled.train_weight() * 3 / 2);
        std::cout << "  ok the post under Knapford carries what the engine weighs\n";
    }

    {
        assert(engine::total_weight(engine::Id::gordon) > engine::total_weight(engine::Id::james));
        assert(engine::total_weight(engine::Id::james) > engine::total_weight(engine::Id::edward));
        assert(engine::total_weight(engine::Id::percy) < engine::total_weight(engine::Id::thomas));
        assert(engine::total_weight(engine::Id::henry) > engine::total_weight(engine::Id::james));

        // Every engine appears exactly once in the engine-select order.
        for(int id = 0; id < engine::engine_count(); ++id)
        {
            assert(engine::ROSTER[engine::roster_index(engine::Id(id))] == engine::Id(id));
        }

        // Edward banks every train on Gordon's Hill and nowhere else.
        bridge::BridgeSystem hill;
        hill.load_level(1);
        hill.set_engine(engine::Id::thomas);
        assert(hill.car_kind(hill.axle_count() - 1) == engine::CarKind::banker);
        assert(hill.train_total_weight() ==
               engine::total_weight(engine::Id::thomas) + engine::BANKER_WEIGHT);
        hill.load_level(0);
        assert(hill.train_total_weight() == engine::total_weight(engine::Id::thomas));

        bridge::BridgeSystem system;
        system.set_engine(engine::Id::gordon);
        assert(system.train_total_weight() == engine::total_weight(engine::Id::gordon));
        assert(system.train_speed() > bn::fixed(1));
        std::cout << "  ok engine weights and coaches\n";
    }

    // --- The rewind ---------------------------------------------------------

    {
        bridge::BridgeSystem system;
        assert(build_bare_deck(system, 0));
        const int built_links = system.struts().size();
        const bn::fixed_point built_joint = system.nodes()[3].position;
        run(system);
        assert(system.status() == bridge::SimStatus::failed_fall);
        assert(system.struts().size() < built_links);
        assert(system.built_struts().size() == built_links);

        system.end_test();
        assert(system.status() == bridge::SimStatus::idle);
        assert(system.struts().size() == built_links);
        assert(system.nodes()[3].position == built_joint);

        bool any_overworked = false;

        for(const bridge::Strut& strut : system.struts())
        {
            any_overworked = any_overworked ||
                             bridge::strut_peak_heat(strut) == bridge::Heat::hot;
        }

        assert(any_overworked);
        std::cout << "  ok a failed test rewinds the build and marks the links that gave way\n";
    }

    // --- Cable, heat, and picking a link -----------------------------------

    {
        // A cable holds what hangs from it and nothing that pushes on it: the
        // same link that carries the Knapford joint from above as a hanger is
        // useless as a prop from below.
        bridge::BridgeSystem hung;
        hung.load_level(1);
        hung.set_engine(engine::Id::thomas);
        assert(level_solutions::build(hung, level_solutions::reference(1)));
        run(hung);
        assert(hung.status() == bridge::SimStatus::success);

        for(const bridge::Strut& strut : hung.struts())
        {
            if(strut.role == bridge::LinkRole::cable)
            {
                assert(strut.peak_tension > bn::fixed(0));
            }
        }

        // Wellsworth's par design props the deck from ledges below. Swap the
        // rods for cables and there is nothing holding it up.
        const level_solutions::Design& design = level_solutions::reference(4);
        bridge::BridgeSystem propped;
        propped.load_level(4);
        propped.set_engine(level::get(4).scheduled_engine);

        for(int i = 0; i < design.joint_count; ++i)
        {
            assert(propped.add_node(design.joints[i].x, design.joints[i].y, false));
        }

        for(int i = 0; i < design.link_count; ++i)
        {
            const level_solutions::Link& link = design.links[i];
            const bridge::LinkRole role =
                link.role == bridge::LinkRole::brace ? bridge::LinkRole::cable : link.role;
            assert(propped.add_strut(link.a, link.b, role));
        }

        run(propped);
        assert(propped.status() == bridge::SimStatus::failed_fall);
        assert(bridge::CABLE.cost < bridge::ROD.cost);
        std::cout << "  ok cable hangs a deck but cannot prop one\n";
    }

    {
        assert(bridge::heat_for(3, 8) == bridge::Heat::calm);
        assert(bridge::heat_for(4, 8) == bridge::Heat::warm);
        assert(bridge::heat_for(6, 8) == bridge::Heat::warm);
        assert(bridge::heat_for(bn::fixed(13) / 2, 8) == bridge::Heat::hot);

        bridge::BridgeSystem system;
        assert(level_solutions::build_reference(system, 0));
        assert(system.stress_percent() == 0);
        run(system);
        assert(system.status() == bridge::SimStatus::success);
        assert(system.peak_stress_percent() > 50);
        assert(system.peak_stress_percent() < 100);
        std::cout << "  ok heat bands and the stress readout (Knapford peaks at "
                  << system.peak_stress_percent() << "%)\n";
    }

    {
        bridge::BridgeSystem system;
        assert(level_solutions::build_reference(system, 0));
        // The post runs from the joint at (0, 12) down to the rock at (0, 40).
        const bn::optional<int> post = system.find_strut_near(bn::fixed_point(2, 26));
        assert(post.has_value());
        assert(system.struts()[post.value()].node_a == 2);
        assert(!system.find_strut_near(bn::fixed_point(20, 30)).has_value());
        assert(system.remove_strut(post.value()));
        assert(system.struts().size() == 2);
        assert(!system.remove_strut(7));
        std::cout << "  ok a link can be picked by pointing at it and removed\n";
    }

    std::cout << "All host physics tests passed.\n";
    return 0;
}
