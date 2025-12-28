#include <fsm/runtime.hpp>
#include <catch2/catch_test_macros.hpp>
#include <sstream>

enum class Light { Red, Green, Yellow };
enum class Event { Timer };

struct Context {
    int counter = 0;
};

TEST_CASE("runtime basic transition flow", "[fsm]") {
    fsm::runtime<Light, Event, Context> sm(Light::Red);

    sm.add_transition({ Light::Red, Event::Timer, Light::Green, nullptr,
        [](Context& ctx){ ctx.counter++; } });
    sm.add_transition({ Light::Green, Event::Timer, Light::Yellow, nullptr,
        [](Context& ctx){ ctx.counter++; } });
    sm.add_transition({ Light::Yellow, Event::Timer, Light::Red, nullptr,
        [](Context& ctx){ ctx.counter++; } });

    Context ctx;
    REQUIRE(sm.current() == Light::Red);
    REQUIRE(sm.dispatch(Event::Timer, ctx) == fsm::runtime<Light, Event, Context>::Result::Ok);
    REQUIRE(sm.current() == Light::Green);
    REQUIRE(ctx.counter == 1);
    REQUIRE(sm.dispatch(Event::Timer, ctx) == fsm::runtime<Light, Event, Context>::Result::Ok);
    REQUIRE(sm.current() == Light::Yellow);
    REQUIRE(ctx.counter == 2);
    REQUIRE(sm.dispatch(Event::Timer, ctx) == fsm::runtime<Light, Event, Context>::Result::Ok);
    REQUIRE(sm.current() == Light::Red);
    REQUIRE(ctx.counter == 3);
}

TEST_CASE("runtime guard rejection", "[fsm]") {
    fsm::runtime<Light, Event, Context> sm(Light::Red);
    bool guard_called = false;
    sm.add_transition({ Light::Red, Event::Timer, Light::Green,
        [&](const Context&) { guard_called = true; return false; },
        nullptr });
    Context ctx;
    auto res = sm.dispatch(Event::Timer, ctx);
    REQUIRE(res == fsm::runtime<Light, Event, Context>::Result::GuardRejected);
    REQUIRE(sm.current() == Light::Red);
    REQUIRE(guard_called);
}

TEST_CASE("dot generation", "[fsm]") {
    fsm::runtime<Light, Event, Context> sm(Light::Red);
    sm.add_transition({ Light::Red, Event::Timer, Light::Green, nullptr, nullptr });
    std::string dot = sm.to_dot();
    // Basic sanity checks – ensure nodes and edge appear
    // Enum values are printed as their underlying integer representations (Red=0, Green=1, Timer=0).
    REQUIRE(dot.find("0") != std::string::npos);
    REQUIRE(dot.find("1") != std::string::npos);
    // Ensure the edge label (event) appears (also "0") and an arrow is present.
    REQUIRE(dot.find("->") != std::string::npos);
    // The graph should start with "digraph FSM"
    REQUIRE(dot.rfind("digraph FSM", 0) == 0);
}
