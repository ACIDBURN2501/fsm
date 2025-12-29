#include <fsm/runtime.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <cstdlib>

enum class Light { Red, Green, Yellow };
enum class Event { Timer };

TEST_CASE("dot graph generation and validation", "[fsm][dot]") {
    // Build a simple FSM
    fsm::runtime<Light, Event> sm(Light::Red);
    sm.add_transition({ Light::Red,    Event::Timer, Light::Green,  nullptr, nullptr });
    sm.add_transition({ Light::Green,  Event::Timer, Light::Yellow, nullptr, nullptr });
    sm.add_transition({ Light::Yellow, Event::Timer, Light::Red,    nullptr, nullptr });

    // Get DOT representation
    std::string dot = sm.to_dot();

    // Write to a temporary file
    const std::string dotFile = "fsm_test.dot";
    std::ofstream out(dotFile);
    REQUIRE(out.is_open());
    out << dot;
    out.close();

    // Use Graphviz to render the DOT file (requires `dot` in PATH)
    const std::string pngFile = "fsm_test.png";
    std::string cmd = std::string("dot -Tpng ") + dotFile + " -o " + pngFile;
    int ret = std::system(cmd.c_str());
    // Graphviz should exit with status 0
    REQUIRE(ret == 0);

    // Basic sanity: the PNG file should now exist and be non‑empty
    std::ifstream png(pngFile, std::ios::binary | std::ios::ate);
    REQUIRE(png.is_open());
    REQUIRE(png.tellg() > 0);
    png.close();

    // Cleanup (optional)
    std::remove(dotFile.c_str());
    std::remove(pngFile.c_str());
}
