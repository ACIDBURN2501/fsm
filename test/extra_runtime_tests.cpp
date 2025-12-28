// Extra unit tests to increase coverage of fsm::runtime
// These tests cover additional edge cases not exercised by existing suite.

#include <fsm/runtime.hpp>
#include <catch2/catch_test_macros.hpp>

// Test 1: Overwriting an existing transition (same src+event) should replace the previous one.
TEST_CASE("transition overwrite replaces previous entry", "[fsm]") {
    enum class State { A, B, C };
    enum class Event { X };

    fsm::runtime<State, Event> sm(State::A);
    // First transition: A --X--> B
    sm.add_transition({ State::A, Event::X, State::B, nullptr, nullptr });
    // Overwrite with a new destination: A --X--> C
    sm.add_transition({ State::A, Event::X, State::C, nullptr, nullptr });

    // Dispatch should follow the latest added transition (to C)
    REQUIRE(sm.dispatch(Event::X) == fsm::runtime<State, Event>::Result::Ok);
    REQUIRE(sm.current() == State::C);
}

// Test 2: Void context version with guard and action.
TEST_CASE("void context guard and action handling", "[fsm]") {
    enum class State { Start, End };
    enum class Event { Go };

    static int action_counter = 0;
    auto guard_true = []() { return true; };
    auto guard_false = []() { return false; };
    auto action = []() { ++action_counter; };

    fsm::runtime<State, Event> sm(State::Start);
    // Transition with guard that returns true and an action.
    sm.add_transition({ State::Start, Event::Go, State::End, guard_true, action });

    // Guard true: should perform action and transition.
    REQUIRE(sm.dispatch(Event::Go) == fsm::runtime<State, Event>::Result::Ok);
    REQUIRE(sm.current() == State::End);
    REQUIRE(action_counter == 1);

    // Reset to Start for guard false case.
    sm = fsm::runtime<State, Event>(State::Start);
    sm.add_transition({ State::Start, Event::Go, State::End, guard_false, action });
    // Guard false: no transition, no action.
    REQUIRE(sm.dispatch(Event::Go) == fsm::runtime<State, Event>::Result::GuardRejected);
    REQUIRE(sm.current() == State::Start);
    REQUIRE(action_counter == 1); // unchanged
}

// Test 3: Using integral (non‑enum) types for state and event.
TEST_CASE("integral type states and events work", "[fsm]") {
    using State = uint8_t; // 0,1,2
    using Event = uint8_t; // 0,1

    fsm::runtime<State, Event> sm(static_cast<State>(0)); // start at 0
    // Transition 0 --0--> 1
    sm.add_transition({ static_cast<State>(0), static_cast<Event>(0), static_cast<State>(1), nullptr, nullptr });
    // Transition 1 --1--> 2
    sm.add_transition({ static_cast<State>(1), static_cast<Event>(1), static_cast<State>(2), nullptr, nullptr });

    // Dispatch first event
    REQUIRE(sm.dispatch(static_cast<Event>(0)) == fsm::runtime<State, Event>::Result::Ok);
    REQUIRE(sm.current() == static_cast<State>(1));
    // Dispatch second event
    REQUIRE(sm.dispatch(static_cast<Event>(1)) == fsm::runtime<State, Event>::Result::Ok);
    REQUIRE(sm.current() == static_cast<State>(2));
}
