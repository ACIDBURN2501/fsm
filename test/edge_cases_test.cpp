#include <fsm/runtime.hpp>
#include <catch2/catch_test_macros.hpp>

enum class State { Locked, Unlocked };
enum class Event { Push, Coin };

TEST_CASE("missing transition handling", "[fsm]") {
    fsm::runtime<State, Event> sm(State::Locked);
    
    // No transitions added yet
    SECTION("dispatch with empty table") {
        REQUIRE(sm.dispatch(Event::Push) == fsm::runtime<State, Event>::Result::NoTransition);
        REQUIRE(sm.current() == State::Locked);
    }

    SECTION("dispatch with partial table") {
        sm.add_transition({ State::Locked, Event::Coin, State::Unlocked, nullptr, nullptr });
        
        // Exists
        REQUIRE(sm.dispatch(Event::Coin) == fsm::runtime<State, Event>::Result::Ok);
        // Does not exist
        REQUIRE(sm.dispatch(Event::Coin) == fsm::runtime<State, Event>::Result::NoTransition);
        REQUIRE(sm.current() == State::Unlocked);
    }
}

TEST_CASE("multiple FSM instances", "[fsm]") {
    fsm::runtime<State, Event> sm1(State::Locked);
    fsm::runtime<State, Event> sm2(State::Unlocked);

    sm1.add_transition({ State::Locked, Event::Coin, State::Unlocked, nullptr, nullptr });
    sm2.add_transition({ State::Unlocked, Event::Push, State::Locked, nullptr, nullptr });

    REQUIRE(sm1.current() == State::Locked);
    REQUIRE(sm2.current() == State::Unlocked);

    sm1.dispatch(Event::Coin);

    REQUIRE(sm1.current() == State::Unlocked);
    REQUIRE(sm2.current() == State::Unlocked); // Should remain unchanged
}
