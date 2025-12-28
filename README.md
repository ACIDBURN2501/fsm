# fsm

A **lightweight, header‑only** finite state machine (FSM) library written in modern C++20.

## Features

- Runtime, table‑driven FSM core (`fsm::runtime`).
- Guard predicates and entry/exit actions.
- Header‑only `INTERFACE` CMake target – easy to consume.
- Dot graph (GraphViz) generation via `to_dot`.

## Getting Started

```bash
# Clone the repo
git clone https://github.com/yourname/fsm.git
cd fsm

# Configure & build (Release)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run the tests
ctest --test-dir build --output-on-failure
```

## Basic Usage (runtime FSM)

```cpp
#include <fsm/runtime.hpp>

enum class Light { Red, Green, Yellow };
enum class Event { Timer };

struct Context { int counter = 0; };

int main() {
    fsm::runtime<Light, Event, Context> sm(Light::Red);
    sm.add_transition({ Light::Red,    Event::Timer, Light::Green, nullptr,
        [](Context& ctx){ ++ctx.counter; } });
    sm.add_transition({ Light::Green,  Event::Timer, Light::Yellow, nullptr,
        [](Context& ctx){ ++ctx.counter; } });
    sm.add_transition({ Light::Yellow, Event::Timer, Light::Red, nullptr,
        [](Context& ctx){ ++ctx.counter; } });

    Context ctx;
    sm.dispatch(Event::Timer, ctx); // Red→Green
    // ...
    std::cout << sm.to_dot(); // visualise with GraphViz
}
```

### Void Context Example

If no context is needed, you can omit the third template parameter:

```cpp
fsm::runtime<Light, Event> sm(Light::Red);
sm.add_transition({ Light::Red, Event::Timer, Light::Green, nullptr, nullptr });
sm.dispatch(Event::Timer); // No context object required
```

## Documentation

The library is documented with Doxygen. After building, run:

```bash
cmake --build build --target doc   # generates HTML in build/doc/html
```

