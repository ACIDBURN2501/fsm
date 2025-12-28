/**
 * @file runtime.hpp
 * @brief Runtime (table‑driven) finite state machine core implementation.
 *
 * This header provides a modern C++20 interface for a runtime-driven FSM.
 * Users supply their own `State` and `Event` enum classes (or any
 * integral‑convertible types) and an optional `Context` object that is
 * passed to guard and action callables.
 *
 * The design uses an `unordered_map` keyed by a 64‑bit composite of the
 * source state and event for O(1) lookup.
 *
 * The implementation is deliberately header-only; the class is declared
 * `inline` so that the library can be used as an INTERFACE target in CMake.
 */

#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <utility>
#include <string>
#include <type_traits>

namespace fsm {

/*
 * Forward declaration of helper used by runtime::to_dot
 */
template<class T>
std::string to_string(const T& value);


/**
 * @brief Runtime finite state machine.
 *
 * @tparam State   Enum class (or integral type) identifying states.
 * @tparam Event   Enum class (or integral type) identifying events.
 * @tparam Context User‑defined data that is passed to guard/action callables.
 *
 * The `Context` type defaults to `void` when no external data is needed.
 * In that case guard/action callables receive no arguments.
 */
template <class State, class Event, class Context = void>
class runtime {
public:
    /* ------------------------------------------------------------ */
    /* Types                                                        */
    /* ------------------------------------------------------------ */
    using Guard  = std::function<bool(const Context&)>;   /**< Guard predicate */
    using Action = std::function<void(Context&)>;       /**< Action to execute on transition */

    /**
     * @brief Structure describing a single transition.
     */
    struct Transition {
        State   src;   /**< Source state */
        Event   ev;    /**< Event triggering the transition */
        State   dst;   /**< Destination state */
        Guard   guard; /**< Optional guard, may be empty */
        Action  action;/**< Optional action, may be empty */
    };

    /**
     * @brief Result of a dispatch operation.
     */
    enum class Result {
        Ok,             /**< Transition performed */
        NoTransition,   /**< No matching transition for current state/event */
        GuardRejected   /**< Guard evaluated to false */
    };

    /* ------------------------------------------------------------ */
    /* Construction                                                 */
    /* ------------------------------------------------------------ */
    /**
     * @brief Construct the FSM with an initial state.
     * @param start Initial state of the machine.
     */
    explicit runtime(State start) noexcept : current_(start) {}

    /* ------------------------------------------------------------ */
    /* Transition management                                        */
    /* ------------------------------------------------------------ */
    /**
     * @brief Add a transition to the internal table.
     * @param tr Transition description.
     */
    inline void add_transition(const Transition& tr) {
        table_[key(tr.src, tr.ev)] = tr;
    }

    /**
     * @brief Dispatch an event, optionally using a context.
     *
     * @param ev   Event to dispatch.
     * @param ctx  Context passed to guard/action callables.
     * @return Result indicating success or failure reason.
     */
    inline Result dispatch(Event ev, Context& ctx) {
        auto it = table_.find(key(current_, ev));
        if (it == table_.end()) {
            return Result::NoTransition;
        }
        const auto& tr = it->second;
        if (tr.guard && !tr.guard(ctx)) {
            return Result::GuardRejected;
        }
        if (tr.action) {
            tr.action(ctx);
        }
        current_ = tr.dst;
        return Result::Ok;
    }

    /**
     * @brief Retrieve the current active state.
     * @return Current state value.
     */
    inline State current() const noexcept { return current_; }

    /* ------------------------------------------------------------ */
    /* DOT graph generation                                         */
    /* ------------------------------------------------------------ */
    /**
     * @brief Generate a GraphViz DOT representation of the FSM.
     *
     * The function requires that `State` and `Event` can be converted to
     * `std::string` via `std::to_string` or a user‑provided overload.
     * For enum classes, users can specialise `std::to_string` or provide a
     * custom formatter.
     *
     * @return DOT language string describing states and transitions.
     */
    inline std::string to_dot() const {
        std::string dot = "digraph FSM {\n  rankdir=LR;\n";
        for (const auto& kv : table_) {
            const auto& tr = kv.second;
            dot += "  \"" + to_string(tr.src) + "\" -> \"" +
                   to_string(tr.dst) + "\" [label=\"" +
                   to_string(tr.ev) + "\"];\n";
        }
        dot += "}\n";
        return dot;
    }

private:
    /**
     * @brief Combine state and event into a 64-bit key for the hash map.
     * @param s State value.
     * @param e Event value.
     * @return 64‑bit composite key.
     */
    static constexpr uint64_t key(State s, Event e) noexcept {
        return (static_cast<uint64_t>(static_cast<std::underlying_type_t<State>>(s)) << 32) |
               static_cast<uint64_t>(static_cast<std::underlying_type_t<Event>>(e));
    }

    std::unordered_map<uint64_t, Transition> table_; /**< Transition table */
    State current_;                                 /**< Current active state */
};

/**
 * @brief Helper to convert enums or integral types to string.
 *
 * Users can specialise this overload for their own enum classes.
 */
template <class T>
inline std::string to_string(const T& value) {
    if constexpr (std::is_enum_v<T>) {
        return std::to_string(static_cast<std::underlying_type_t<T>>(value));
    } else {
        return std::to_string(value);
    }
}

} /* namespace fsm */
