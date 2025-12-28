#include <fsm/runtime.hpp>
#include <catch2/catch_test_macros.hpp>

enum class State { A, B };
enum class Event { X };

TEST_CASE("runtime with void context", "[fsm]") {
    fsm::runtime<State, Event, void> sm(State::A);
    sm.add_transition({ State::A, Event::X, State::B, nullptr, nullptr });

    REQUIRE(sm.dispatch(Event::X) == fsm::runtime<State, Event, void>::Result::Ok);
    REQUIRE(sm.current() == State::B);
}
