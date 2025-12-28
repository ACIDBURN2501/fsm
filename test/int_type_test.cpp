#include <fsm/runtime.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("runtime with int types", "[fsm]") {
    fsm::runtime<int, int, int> sm(0);
    sm.add_transition({ 0, 1, 2, nullptr, nullptr });
    int ctx = 0;
    REQUIRE(sm.dispatch(1, ctx) == fsm::runtime<int, int, int>::Result::Ok);
    REQUIRE(sm.current() == 2);
}
