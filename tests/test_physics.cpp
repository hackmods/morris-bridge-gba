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
        expect_near(physics::max_tension_for_length(physics::SAFE_LENGTH),
                    physics::BASE_MAX_TENSION, bn::fixed(1) / 16,
                    "rating at SAFE_LENGTH");

        // Double the span, half the rating: 96px -> 4, which equals BASE_WEIGHT.
        expect_near(physics::max_tension_for_length(96), 4, bn::fixed(1) / 8,
                    "long span is weaker");
    }

    {
        const bn::fixed_point a(0, 0);
        const bn::fixed_point b(10, 0);
        const bn::fixed_point mid = physics::lerp(a, b, bn::fixed(1) / 2);
        expect_near(mid.x(), 5, bn::fixed(1) / 16, "lerp midpoint x");
        expect_near(mid.y(), 0, 0, "lerp midpoint y");
    }

    // Graph: two banks plus a player deck. Short struts must survive a crossing.
    {
        bridge::BridgeSystem system;
        assert(system.nodes().size() >= 2);

        const bn::optional<int> mid = system.add_node(0, 16, false);
        assert(mid.has_value());

        const bn::optional<int> left = system.add_strut(bridge::START_NODE, mid.value());
        const bn::optional<int> right = system.add_strut(mid.value(), bridge::END_NODE);
        assert(left.has_value());
        assert(right.has_value());
        assert(!system.add_strut(bridge::START_NODE, mid.value()).has_value());

        const bridge::SimStatus started = system.begin_test();
        assert(started == bridge::SimStatus::running);
        assert(system.path_length() == 3);

        bool snapped = false;
        int frames = 0;

        while(system.status() == bridge::SimStatus::running && frames < 400)
        {
            system.update();
            ++frames;

            if(system.status() == bridge::SimStatus::failed_fall)
            {
                snapped = true;
                break;
            }
        }

        std::cout << "  short two-span bridge: frames=" << frames
                  << " status=" << int(system.status())
                  << " snapped=" << snapped << '\n';
        assert(system.status() == bridge::SimStatus::success);
        assert(!snapped);
        std::cout << "  ok two-span deck carries the train\n";

        system.end_test();
        assert(system.status() == bridge::SimStatus::idle);
        assert(system.struts().size() == 2);
    }

    // A single 176px span is rated below BASE_WEIGHT and must snap.
    {
        bridge::BridgeSystem system;
        const bn::optional<int> beam = system.add_strut(bridge::START_NODE, bridge::END_NODE);
        assert(beam.has_value());

        const bn::fixed rating = system.struts()[beam.value()].max_tension;
        assert(rating < physics::BASE_WEIGHT);

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
        assert(system.anchored_node_count() == 8);
        assert(system.player_node_budget() == 8);

        const bn::optional<int> mid = system.add_node(0, 16, false);
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

    std::cout << "All host physics tests passed.\n";
    return 0;
}
