/**
 * @file tests/test_physics.cpp
 *
 * Host unit tests for Ryan's Bridge fixed-point distance and tension logic.
 *
 * Compiles with stock GCC/Clang (no devkitARM, no Butano). The `tests/host_bn`
 * directory mocks `bn::fixed` as 20.12 with the same int64_t shift/divide
 * pattern the engine uses, then links the real `src/physics.cpp` and
 * `src/bridge_system.cpp`.
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
#include "run_score.h"

namespace
{
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
} // namespace

int main()
{
    std::cout << "Morris Bridge host physics tests (bn::fixed mock is 20.12)\n";

    // 3-4-5 triangle: exact integer length, no rounding alibi.
    {
        const bn::fixed_point a(0, 0);
        const bn::fixed_point b(3, 4);
        const bn::fixed d2 = physics::squared_distance(a, b);
        const bn::fixed d = physics::distance(a, b);
        expect_near(d2, 25, 0, "squared_distance(3,4)");
        expect_near(d, 5, bn::fixed(1) / 16, "distance(3,4) ~ 5");
    }

    {
        const bn::fixed root = physics::approx_sqrt(100);
        expect_near(root, 10, bn::fixed(1) / 16, "approx_sqrt(100)");
        expect_near(physics::approx_sqrt(0), 0, 0, "approx_sqrt(0)");
        expect_near(physics::approx_sqrt(-3), 0, 0, "approx_sqrt(negative)");
    }

    // Tension = BaseWeight × (DistanceFromTrain / StrutLength)
    {
        const bn::fixed weight = 10;
        const bn::fixed length = 100;
        expect_near(physics::tension(weight, 0, length), 0, 0, "tension at d=0");
        expect_near(physics::tension(weight, 50, length), 5, bn::fixed(1) / 16,
                    "tension at midspan (X=50, L=100, W=10)");
        expect_near(physics::tension(weight, 100, length), 10, bn::fixed(1) / 16,
                    "tension at far end equals W");
        expect_near(physics::tension(weight, 30, length), 3, bn::fixed(1) / 16,
                    "train at X=30 on L=100, W=10 => T=3");
        expect_near(physics::tension(weight, 40, 0), 0, 0, "zero-length strut");
    }

    {
        assert(!physics::exceeds_max_tension(4, 4) && "equal tension must hold");
        assert(physics::exceeds_max_tension(5, 4) && "over-limit must snap");
        std::cout << "  ok exceeds_max_tension edges\n";
    }

    {
        // A SAFE_LENGTH beam is rated BASE_MAX_TENSION.
        expect_near(physics::base_max_tension_for_length(physics::SAFE_LENGTH),
                    physics::BASE_MAX_TENSION, bn::fixed(1) / 16,
                    "rating at SAFE_LENGTH");

        // Double the span, half the rating: 96px -> 4, which equals BASE_WEIGHT.
        expect_near(physics::base_max_tension_for_length(96), 4, bn::fixed(1) / 8,
                    "long span is weaker");
    }

    {
        expect_near(physics::reinforcement_multiplier(1, 1), 1, 0, "single joints");
        expect_near(physics::reinforcement_multiplier(3, 2), bn::fixed(7) / 4, bn::fixed(1) / 16,
                    "reinforced joint");
        expect_near(physics::effective_max_tension(48, 2, 2, true), 18, bn::fixed(1) / 8,
                    "triangulated deck is stronger");
    }

    {
        const bn::fixed_point a(0, 0);
        const bn::fixed_point b(10, 0);
        const bn::fixed_point mid = physics::lerp(a, b, bn::fixed(1) / 2);
        expect_near(mid.x(), 5, bn::fixed(1) / 16, "lerp midpoint x");
        expect_near(mid.y(), 0, 0, "lerp midpoint y");
    }

    // Graph: deck with three short spans — engine crosses the first 44 px leg.
    {
        bridge::BridgeSystem system;
        assert(system.nodes().size() >= 2);

        const bn::optional<int> mid_a = system.add_node(-44, 52, false);
        const bn::optional<int> mid_b = system.add_node(44, 52, false);
        assert(mid_a.has_value());
        assert(mid_b.has_value());

        const bn::optional<int> left =
            system.add_strut(bridge::START_NODE, mid_a.value());
        const bn::optional<int> deck = system.add_strut(mid_a.value(), mid_b.value());
        const bn::optional<int> right = system.add_strut(mid_b.value(), bridge::END_NODE);
        assert(left.has_value());
        assert(deck.has_value());
        assert(right.has_value());

        const bridge::SimStatus started = system.begin_test();
        assert(started == bridge::SimStatus::running);
        assert(system.path_length() == 4);

        bool snapped = false;
        int frames = 0;

        while(system.status() == bridge::SimStatus::running && frames < 600)
        {
            system.update();
            ++frames;

            if(system.status() == bridge::SimStatus::failed_fall)
            {
                snapped = true;
                break;
            }
        }

        std::cout << "  short three-span bridge: frames=" << frames
                  << " status=" << int(system.status())
                  << " snapped=" << snapped << '\n';
        assert(system.status() == bridge::SimStatus::success);
        assert(!snapped);
        std::cout << "  ok reinforced deck carries the locomotive\n";

        system.end_test();
        assert(system.status() == bridge::SimStatus::idle);
        assert(system.struts().size() == 3);
    }

    // A single 176px span is rated below BASE_WEIGHT and must snap.
    {
        bridge::BridgeSystem system;
        const bn::optional<int> beam = system.add_strut(bridge::START_NODE, bridge::END_NODE);
        assert(beam.has_value());

        const bn::fixed rating = system.struts()[beam.value()].max_tension;
        assert(rating < physics::ENGINE_WEIGHT);

        (void)system.begin_test();

        int frames = 0;

        while(system.status() == bridge::SimStatus::running && frames < 400)
        {
            system.update();
            ++frames;
        }

        std::cout << "  long single span: frames=" << frames
                  << " rating.data=" << rating.data()
                  << " status=" << int(system.status()) << '\n';
        assert(system.status() == bridge::SimStatus::failed_fall);
        assert(system.struts().empty());
        std::cout << "  ok overlong strut snaps under the train\n";
    }

    {
        bridge::BridgeSystem system;
        const bridge::SimStatus started = system.begin_test();
        assert(started == bridge::SimStatus::no_path);
        std::cout << "  ok disconnected banks report no_path\n";
    }

    {
        bridge::BridgeSystem system;
        assert(system.anchored_node_count() == 6);
        assert(system.player_node_budget() == 10);

        const bn::optional<int> mid = system.add_node(0, 52, false);
        assert(mid.has_value());
        assert(system.player_node_count() == 1);

        const bridge::StrutPreview preview = system.preview_strut(bridge::START_NODE, mid.value());
        assert(preview.status == bridge::StrutPreviewStatus::ok);
        assert(preview.length > 0);
        assert(preview.max_tension > 0);

        const bn::optional<int> beam = system.add_strut(bridge::START_NODE, mid.value());
        assert(beam.has_value());
        assert(system.preview_strut(bridge::START_NODE, mid.value()).status ==
               bridge::StrutPreviewStatus::duplicate);

        assert(system.undo_last_edit() == bridge::UndoResult::removed_strut);
        assert(system.struts().empty());
        assert(system.undo_last_edit() == bridge::UndoResult::nothing);

        const bn::optional<int> solo = system.add_node(-8, 24, false);
        assert(solo.has_value());
        assert(system.undo_last_edit() == bridge::UndoResult::removed_node);
        assert(system.remove_player_node(bridge::START_NODE) == bridge::RemoveNodeResult::anchored);
        std::cout << "  ok undo, preview, and delete helpers\n";
    }

    // Heavy engine (Gordon) snaps a span Percy survives.
    {
        bridge::BridgeSystem light;
        bridge::BridgeSystem heavy;
        light.set_engine(engine::Id::percy);
        heavy.set_engine(engine::Id::gordon);
        assert(light.train_weight() == bn::fixed(3));
        assert(heavy.train_weight() == bn::fixed(10));

        const bn::optional<int> mid = light.add_node(0, 16, false);
        assert(mid.has_value());
        assert(heavy.add_node(0, 16, false).has_value());

        assert(light.add_strut(bridge::START_NODE, mid.value()).has_value());
        assert(light.add_strut(mid.value(), bridge::END_NODE).has_value());
        assert(heavy.add_strut(bridge::START_NODE, mid.value()).has_value());
        assert(heavy.add_strut(mid.value(), bridge::END_NODE).has_value());

        (void)light.begin_test();
        int frames = 0;

        while(light.status() == bridge::SimStatus::running && frames < 400)
        {
            light.update();
            ++frames;
        }

        assert(light.status() == bridge::SimStatus::success);
        std::cout << "  ok Percy (weight 3) crosses two-span deck\n";

        (void)heavy.begin_test();
        frames = 0;

        while(heavy.status() == bridge::SimStatus::running && frames < 400)
        {
            heavy.update();
            ++frames;
        }

        assert(heavy.status() == bridge::SimStatus::failed_fall);
        std::cout << "  ok Gordon (weight 10) snaps the same deck\n";
    }

    // Gordon with two coaches loads harder than James with one.
    {
        bridge::BridgeSystem gordon;
        gordon.set_engine(engine::Id::gordon);
        const bn::fixed gordon_end = gordon.effective_weight_at(bn::fixed(96), bn::fixed(96));
        bridge::BridgeSystem james;
        james.set_engine(engine::Id::james);
        const bn::fixed james_end = james.effective_weight_at(bn::fixed(96), bn::fixed(96));
        assert(gordon_end > james_end);
        std::cout << "  ok coach load ramp increases with engine coaches\n";
    }

    {
        bridge::BridgeSystem system;
        system.load_level(0);
        assert(system.anchored_node_count() == 6);
        assert(system.nodes()[bridge::START_NODE].position.x().integer() == -88);
        assert(system.nodes()[bridge::START_NODE].position.y().integer() == 52);
        assert(system.level_def().deck_y.integer() == 52);

        system.load_level(1);
        assert(system.anchored_node_count() == 6);
        assert(system.nodes()[bridge::START_NODE].position.y().integer() == 44);
        assert(system.level_def().deck_y.integer() == 44);

        system.load_level(2);
        assert(system.anchored_node_count() == 6);
        assert(system.nodes()[bridge::END_NODE].position.x().integer() == 104);

        system.load_level(-1);
        assert(system.current_level() == 0);
        system.load_level(99);
        assert(system.current_level() == level::COUNT - 1);
        std::cout << "  ok per-level seeding and clamping\n";
    }

    {
        bridge::BridgeSystem system;
        const bridge::StrutPreview same_node = system.preview_strut(bridge::START_NODE, bridge::START_NODE);
        assert(same_node.status == bridge::StrutPreviewStatus::same_node);

        const bridge::StrutPreview invalid = system.preview_strut(bridge::START_NODE, 99);
        assert(invalid.status == bridge::StrutPreviewStatus::invalid_node);

        const bn::optional<int> mid = system.add_node(0, 24, false);
        assert(mid.has_value());
        assert(system.preview_strut(bridge::START_NODE, mid.value()).status ==
               bridge::StrutPreviewStatus::ok);
        std::cout << "  ok strut preview statuses\n";
    }

    {
        expect_near(physics::max_tension_for_length(48), physics::BASE_MAX_TENSION,
                    bn::fixed(1) / 16, "rating at 48px span");
        expect_near(physics::max_tension_for_length(176),
                    physics::BASE_MAX_TENSION.safe_multiplication(physics::SAFE_LENGTH).safe_division(176),
                    bn::fixed(1) / 8, "rating at 176px span");
        std::cout << "  ok max_tension_for_length boundaries\n";
    }

    {
        bridge::BridgeSystem system;
        const bn::optional<int> first = system.add_node(-24, 24, false);
        const bn::optional<int> second = system.add_node(24, 24, false);
        assert(first.has_value());
        assert(second.has_value());
        assert(system.add_strut(bridge::START_NODE, first.value()).has_value());
        assert(system.add_strut(first.value(), second.value()).has_value());
        assert(system.add_strut(second.value(), bridge::END_NODE).has_value());
        assert(system.begin_test() == bridge::SimStatus::running);

        int frames = 0;

        while(system.status() == bridge::SimStatus::running && frames < 500)
        {
            system.update();
            ++frames;
        }

        assert(system.status() == bridge::SimStatus::success);
        std::cout << "  ok three-span bridge carries the train\n";
    }

    {
        bridge::BridgeSystem system;
        const int budget = system.player_node_budget();

        for(int i = 0; i < budget; ++i)
        {
            assert(system.add_node(-8 + i * 4, 24, false).has_value());
        }

        assert(!system.add_node(0, 32, false).has_value());
        std::cout << "  ok player node budget is enforced\n";
    }

    {
        bridge::BridgeSystem system;
        system.load_level(0);
        assert(!system.has_track_path());
        assert(level_solutions::build_knapford_reference(system));
        assert(system.has_track_path());
        std::cout << "  ok track path is required before a test run\n";
    }

    {
        bridge::BridgeSystem system;
        system.load_level(1);
        assert(system.add_strut(bridge::START_NODE, bridge::END_NODE, bridge::LinkRole::brace));
        assert(system.begin_test() == bridge::SimStatus::no_path);
        std::cout << "  ok brace-only link is not a train route\n";
    }

    {
        bridge::BridgeSystem system;
        system.load_level(1);
        const bridge::StrutPreview timber =
            system.preview_strut(bridge::START_NODE, 3, bridge::LinkRole::track);
        const bridge::StrutPreview rod =
            system.preview_strut(bridge::START_NODE, 3, bridge::LinkRole::brace);
        assert(timber.status == bridge::StrutPreviewStatus::ok);
        assert(rod.status == bridge::StrutPreviewStatus::ok);
        assert(rod.max_tension > timber.max_tension);
        std::cout << "  ok rod material is stronger than timber at the same span\n";
    }

    {
        bridge::BridgeSystem system;
        system.load_level(0);
        system.set_engine(engine::Id::thomas);
        const bn::fixed thomas_load = system.effective_weight_at(bn::fixed(256), bn::fixed(256));
        assert(system.add_strut(bridge::START_NODE, bridge::END_NODE).has_value());
        assert(bridge::strut_warns_for_load(system.struts()[0], thomas_load));

        const bn::optional<int> near_a = system.add_node(-8, 52, false);
        const bn::optional<int> near_b = system.add_node(16, 52, false);
        assert(near_a.has_value());
        assert(near_b.has_value());
        assert(system.add_strut(near_a.value(), near_b.value()).has_value());
        assert(!bridge::strut_warns_for_load(system.struts()[1], thomas_load));

        bridge::BridgeSystem hill;
        hill.load_level(1);
        const bn::fixed timber_ghost =
            hill.span_preview_capacity(physics::SAFE_LENGTH * 4, bridge::LinkRole::track);
        const bn::fixed rod_ghost =
            hill.span_preview_capacity(physics::SAFE_LENGTH * 4, bridge::LinkRole::brace);
        assert(rod_ghost > timber_ghost);
        std::cout << "  ok build-mode stress preview warns on overlong timber\n";
    }

    {
        const char* names[] = {
            "Knapford", "Gordon's Hill", "Vicarstown", "Tidmouth",
            "Wellsworth", "Crovan's Gate", "Callan Canal", "Sodor Summit",
        };

        for(int level_index = 0; level_index < level::COUNT; ++level_index)
        {
            bridge::BridgeSystem system;
            assert(level_solutions::run_level_playthrough(system, level_index));
            const session::RunScore score =
                session::compute_run_score(system, system.level_def());
            assert(score.stars == 3);
            assert(score.total_cost <= system.level_def().par_cost);
            std::cout << "  ok " << names[level_index] << " reference scores 3 stars (cost="
                      << score.total_cost << " par=" << system.level_def().par_cost << ")\n";
        }
    }

    std::cout << "All host physics tests passed.\n";
    return 0;
}
