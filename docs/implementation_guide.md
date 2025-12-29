# FSM Runtime – Implementation & Usage Guide

*Generated documentation for the header-only finite‑state‑machine library located in `include/fsm/runtime.hpp`.*

---

## Table of Contents
1. [High-level Overview](#high-level-overview)
2. [Core Concepts](#core-concepts)
   - [Template Parameters](#template-parameters)
   - [Guard / Action Types](#guard--action-types)
   - [Transition Representation](#transition-representation)
   - [Composite Key & Transition Table](#composite-key--transition-table)
3. [Defining the FSM (Initialization)](#defining-the-fsm-initialization)
   - [Example State / Event enums](#example-state--event-enums)
   - [Adding Transitions (with guards & actions)](#adding-transitions-with-guards--actions)
4. [Dispatching Events](#dispatching-events)
   - [Result enum](#result-enum)
   - [Dispatcher flow (with and without context)](#dispatcher-flow-with-and-without-context)
5. [Using a Context Object (Stateful Behaviour)](#using-a-context-object-stateful-behaviour)
6. [ISR / Real-time Friendly Usage](#isr--real-time-friendly-usage)
   - [Guidelines for ISR safety](#guidelines-for-isr-safety)
   - *Deferred handling pattern* example
7. [Cyclic / Hold Transitions](#cyclic--hold-transitions)
8. [DOT Graph Generation (Visualization)](#dot-graph-generation-visualization)
   - [Custom `to_string` overloads](#custom-tostring-overloads)
9. [Building & Testing the Library](#building--testing-the-library)
10. [Common Pitfalls & Best Practices](#common-pitfalls--best-practices)
11. [Complete Minimal Example (ISR-friendly)](#complete-minimal-example-isr-friendly)
12. [License & Attribution](#license--attribution)

---

## High-level Overview
`fsm::runtime` provides a **header-only, table‑driven finite‑state‑machine** that works with any user‑defined `State` and `Event` types (typically `enum class`).
It supports:
* **Guards** – predicates that can block a transition.
* **Actions** – side-effects executed when a transition succeeds.
* An optional **Context** object that can be read by guards and mutated by actions.
* O(1) lookup of transitions via a 64-bit composite key.
* Generation of a GraphViz DOT representation for visual debugging.

---

## Core Concepts

### Template Parameters
```cpp
template <class State, class Event, class Context = void>
class runtime { … };
```
| Parameter | Description |
|---|---|
| **State** | Enum (or integral) identifying a state. |
| **Event** | Enum (or integral) identifying an event that triggers transitions. |
| **Context** | User-defined data passed to guard/action callables. Use `void` for a completely stateless FSM. |

### Guard / Action Types
The helper struct `fsm_types<Context>` defines:
```cpp
using Guard  = std::function<bool(const Context&)>; // non-void
using Action = std::function<void(Context&)>;       // non-void
```
A partial specialization for `void` turns them into `std::function<bool()>` and `std::function<void()>` respectively, so you never need to provide a dummy argument.

### Transition Representation
```cpp
struct Transition {
    State   src;   // source state
    Event   ev;    // triggering event
    State   dst;   // destination state
    Guard   guard; // optional guard (may be empty)
    Action  action;// optional action (may be empty)
};
```
A transition is a row in the internal table.  Guards are evaluated *before* actions; if a guard fails the state remains unchanged.

### Composite Key & Transition Table
The library builds a 64-bit key:
```cpp
static constexpr uint64_t key(State s, Event e) noexcept {
    uint64_t s_val = …; // static_cast to underlying integer
    uint64_t e_val = …;
    return ((s_val & 0xFFFFFFFFULL) << 32) | (e_val & 0xFFFFFFFFULL);
}
```
The key is used in:
```cpp
std::unordered_map<uint64_t, Transition> table_; // O(1) lookup
```
Because the map only stores the key → `Transition` mapping, the runtime overhead is minimal.

---

## Defining the FSM (Initialization)
All transitions should be added **once** during system start-up.  This guarantees deterministic behaviour and avoids dynamic allocations in time‑critical contexts.

### Example State / Event enums
```cpp
enum class Light { Red, Green, Yellow };
enum class Button { Press };
```

### Adding Transitions (with guards & actions)
```cpp
fsm::runtime<Light, Button, MyContext> fsm{ Light::Red };

fsm.add_transition({
    Light::Red,   Button::Press, Light::Green,
    nullptr,                       // no guard → always allowed
    nullptr                        // no action
});

// Transition with guard and action
fsm.add_transition({
    Light::Green, Button::Press, Light::Yellow,
    [](const MyContext& ctx){ return ctx.can_go_to_yellow; }, // guard
    [](MyContext& ctx){ ctx.counter++; }                     // action
});
```
The `add_transition` call overwrites any existing entry for the same `(src, ev)` pair, mimicking a “last definition wins” rule.

---

## Dispatching Events
Two overloads exist; the compiler selects the correct one based on whether `Context` is `void`.

### Result enum
```cpp
enum class Result { Ok, NoTransition, GuardRejected };
```
The caller can act on these outcomes (e.g., log a guard failure or fallback to a safe state).

### Dispatcher flow (with and without context)
1. Compute composite key from `current_` and supplied `ev`.
2. Look up the transition in `table_`.
3. If not found → `NoTransition`.
4. If a guard exists, invoke it.
   * Guard returns `false` → `GuardRejected`.
5. If an action exists, invoke it.
6. Update `current_` to `dst`.
7. Return `Ok`.

The whole operation is **O(1)** and consists of a map lookup plus up to two `std::function` calls.

---

## Using a Context Object (Stateful Behaviour)
When you need the FSM to read or modify external data (hardware registers, timers, counters, etc.), define a struct and pass it to `dispatch`:
```cpp
struct MyContext {
    bool hw_ready{};
    int  counter{};
    // … any other data the guards/actions need
};

MyContext ctx{ .hw_ready = true };
fsm.dispatch(Button::Press, ctx);
```
Guards receive `const MyContext&` (read-only), while actions receive `MyContext&` and may mutate it.

---

## ISR / Real-time Friendly Usage
### Guidelines for ISR safety
* **No dynamic allocation** – build the transition table before enabling interrupts.
* **Keep guards/actions short** (a few microseconds max). Avoid I/O, blocking calls, or anything that may sleep.
* **Re-entrancy** – avoid static locals inside guards/actions or protect them with `std::atomic`/critical sections.
* **No exceptions** – the library never throws; make sure any code inside guards/actions catches its own exceptions.

### Deferred handling pattern (recommended)
```cpp
static std::atomic<bool> start_requested{false};

extern "C" void button_isr() {
    start_requested.store(true, std::memory_order_relaxed);
}

void loop() {
    if (start_requested.exchange(false)) {
        auto res = fsm.dispatch(Event::Start, ctx);
        // optional: handle GuardRejected / NoTransition
    }
    // other periodic work …
}
```
The ISR only sets a flag – the heavy-weight `dispatch` runs in the main loop where it can safely call complex actions.

---

## Cyclic / Hold Transitions
A *hold* transition simply maps a source state back to itself:
```cpp
fsm.add_transition({
    State::Running, Event::Pause, State::Running, // stay in Running
    [](const MyContext& c){ return c.can_pause; }, // guard
    [](MyContext& c){ c.paused_flag = true; }    // side-effect only
});
```
Guards can therefore “block” (return false) or “allow but stay” (return true with `dst == src`).

---

## DOT Graph Generation (Visualization)
```cpp
std::string dot = fsm.to_dot();
// write `dot` to a .dot file and run: dot -Tpng fsm.dot -o fsm.png
```
The function expects a `to_string` overload for your `State` and `Event` types.  The default implementation prints the underlying integer value, but you can provide a friend/namespace overload for nicer labels:
```cpp
std::string to_string(Light l) {
    switch(l) {
        case Light::Red:    return "Red";
        case Light::Green:  return "Green";
        case Light::Yellow: return "Yellow";
    }
    return "?";
}
```
The generated DOT file contains one node per state and one directed edge per transition, labelled with the event name.

---

## Building & Testing the Library
The project is header-only, but the test suite (Catch2) validates the behaviour.
```bash
# Configure (Debug for extra warnings)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
# Build tests
cmake --build build -- -j$(nproc)
# Run all tests
ctest --test-dir build --output-on-failure
```
All tests live under `test/`.  Adding new tests is straightforward – just create a new `TEST_CASE` in the `test/` directory.

---

## Common Pitfalls & Best Practices
* **Never modify the transition table after the system is live** – doing so from an ISR can corrupt the unordered map.
* **Guard/action lifetime** – store only `std::function` objects that capture by value or by reference to objects that out-live the FSM.
* **`Context` must outlive every dispatch** – pass the same context (or a reference to a global/static context) each time you call `dispatch`.
* **Avoid using `std::to_string` on user-defined enums without providing an overload** – otherwise the DOT output will show numeric values only.
* **Thread safety** – if you have multiple threads dispatching events, protect the shared context with `std::mutex` or make the FSM itself thread-local.

---

## Complete Minimal Example (ISR-friendly)
```cpp
#include <fsm/runtime.hpp>
#include <atomic>

enum class State { Idle, Running, Paused };
enum class Event { Start, Stop, Pause, Resume };

struct MyContext {
    bool hw_ready{false};
    bool resume_allowed{true};
    // … other fields
};

using FSM = fsm::runtime<State, Event, MyContext>;
static FSM fsm{ State::Idle };
static MyContext ctx;
static std::atomic<bool> start_flag{false};

extern "C" void button_isr() {
    start_flag.store(true, std::memory_order_relaxed);
}

void fsm_init()
{
    // Idle -> Running (guarded by hw_ready)
    fsm.add_transition({
        State::Idle, Event::Start, State::Running,
        [](const MyContext& c){ return c.hw_ready; },
        [](MyContext& c){ /* start motor, reset timers … */ }
    });
    // Running -> Paused (no guard)
    fsm.add_transition({ State::Running, Event::Pause, State::Paused, nullptr, nullptr });
    // Paused -> Running (guarded by resume_allowed)
    fsm.add_transition({
        State::Paused, Event::Resume, State::Running,
        [](const MyContext& c){ return c.resume_allowed; }, nullptr
    });
    // Running -> Idle (stop)
    fsm.add_transition({ State::Running, Event::Stop, State::Idle, nullptr, nullptr });
}

void loop()
{
    if (start_flag.exchange(false)) {
        auto res = fsm.dispatch(Event::Start, ctx);
        // optional: handle GuardRejected / NoTransition
    }
    // other periodic work …
}
```
Compile with the normal CMake workflow; the ISR can be any hardware-specific interrupt routine.

---

*End of document.*
