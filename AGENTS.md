# AGENTS.md – Repository Guidelines for the **fsm** Project

---
## Table of Contents
1. [Build & Test Commands](#build--test-commands)
2. [Lint & Formatting](#lint--formatting)
3. [Code‑Style Guidelines](#code‑style-guidelines)
   - [File Layout & Includes](#file‑layout--includes)
   - [Naming Conventions](#naming-conventions)
   - [Types & Idioms](#types--idioms)
   - [Error Handling & Result Reporting](#error-handling--result-reporting)
   - [Comments & Documentation](#comments--documentation)
   - [C++20 Specifics](#c20-specifics)
4. [Repository‑Specific Rules](#repository‑specific-rules)
5. [Running a Single Test](#running-a-single-test)
6. [Helpful Scripts & Aliases](#helpful-scripts--aliases)
---
## Build & Test Commands
| Goal | Command | Description |
|------|---------|-------------|
| **Configure (Release)** | `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` | Generates a `build/` directory with Release flags. |
| **Configure (Debug)** | `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug` | Enables debug symbols and extra warnings. |
| **Build All Targets** | `cmake --build build -- -j$(nproc)` | Compiles the library, tests, and optional docs. |
| **Run All Tests** | `ctest --test-dir build --output-on-failure` | Executes the Catch2 test suite. |
| **Run Tests (Verbose)** | `ctest --test-dir build -V` | Shows each test’s stdout/stderr. |
| **Run a Single Test Case** | `./build/fsm_tests --test-case "<case name>"` | Uses Catch2’s built‑in filter (see section 5). |
| **Generate Documentation** | `cmake --build build --target doc` | Calls Doxygen; results in `build/doc/html`. |
| **Reformat Sources** | `clang-format -i $(git ls-files "*.hpp" "*.cpp")` | In‑place formatting using the project's `.clang-format`. |
| **Static Analysis** | `run-clang-tidy -p build $(git ls-files "*.hpp" "*.cpp")` | Requires `run-clang-tidy` wrapper; respects `.clang-tidy` if added. |
---
## Lint & Formatting
The project enforces a **strict LLVM‑based style** defined in `.clang-format`:
* **Column limit:** 80 characters.
* **Indentation:** 8‑space width, **no tabs** (`UseTab: Never`).
* **Brace style:** `BreakBeforeBraces: Linux` – opening brace on the same line for functions, after control statements only when required.
* **Trailing comments:** aligned (`AlignTrailingComments: true`).
* **Include order:** `SortIncludes: true` – standard library headers first, then third‑party, then project headers.
* **Consecutive macros/bit‑fields:** aligned across empty lines and comments.
* **Short constructs:** never allow one‑line `if`, `while`, or `for` statements (`AllowShortIfStatementsOnASingleLine: Never`).

**Lint enforcement workflow**:
```bash
# Reformat all source files
clang-format -i $(git ls-files "*.hpp" "*.cpp")
# Optional: run clang-tidy (if a .clang-tidy file is added later)
run-clang-tidy -p build $(git ls-files "*.hpp" "*.cpp")
```
Any CI pipeline should run the above steps and fail on un‑formatted code.
---
## Code‑Style Guidelines
### File Layout & Includes
1. **Header‑only library** – public headers live under `include/fsm/`; implementation files (tests, examples) under `test/` or `examples/`.
2. **Include ordering** (per file):
   ```cpp
   // 1. Standard library
   #include <cstdint>
   #include <functional>
   #include <unordered_map>
   #include <string>

   // 2. Third‑party (e.g., Catch2 in test files)
   #include <catch2/catch_test_macros.hpp>

   // 3. Project headers
   #include <fsm/runtime.hpp>
   ```
3. Every public header must have an **include guard** (`#ifndef … #define … #endif`) and a brief Doxygen file header.
4. Keep each file under **80 lines** when possible; longer files should be split into logical sections with a clear `// -------------------------------------------------------------------` separator.

### Naming Conventions
| Element | Convention | Example |
|---------|------------|---------|
| **Types / Classes** | `snake_case` (for templates/library) or `PascalCase` | `runtime`, `Context` |
| **Enum classes** | `PascalCase` for the enum, `CamelCase` values | `enum class Light { Red, Green };` |
| **Functions / Methods** | `snake_case` | `add_transition`, `dispatch` |
| **Variables / Parameters** | `snake_case` | `current_state`, `event_id` |
| **Constants** | `SCREAMING_SNAKE` or `constexpr` scoped inside a class | `constexpr uint64_t key …` |
| **Templates** | `T`, `State`, `Event`, `Context` – capitalized single letters or descriptive names. |
| **Namespace** | all lower‑case, short – `fsm` |
---
### Types & Idioms
* **Prefer `enum class`** over unscoped enums for type safety.
* **Use `std::underlying_type_t`** when converting enum values to integers (as done in the key generation).
* **Guard & Action callables** are `std::function` accepting a `const Context&` or `Context&`. When no context is required, the `Context` type defaults to `void` and the callables become `std::function<void()>`.
* **Result reporting** – the `runtime::Result` enum provides explicit success/failure states; **never throw** from the core library.
* **Avoid raw pointers**; use `std::unique_ptr` or references where ownership is clear.
* **Prefer `constexpr`** for compile‑time constants (e.g., `key()` function).
---
### Error Handling & Result Reporting
* The FSM core never uses exceptions; it returns a `Result` enum (`Ok`, `NoTransition`, `GuardRejected`).
* **User code** may translate `Result` to exceptions if desired, but the library stays exception‑free.
* Guard functions return `bool`; they must be **pure** (no side effects) and may read from the `Context`.
* Action functions return `void`; they may mutate the `Context`.
* When a transition is missing, `dispatch` returns `Result::NoTransition` – callers should handle it explicitly.
---
### Comments & Documentation
* **File‑level comment**: start each header with a Doxygen block (`/** … */`).
* **Public API**: annotate classes, structs, functions, and enums with `@brief`, `@param`, `@return`.
* **Implementation details** that are not part of the public contract may use ordinary `/* … */` comments.
* **Inline comments** should be aligned and short; avoid trailing whitespace.
* **DOT export** documentation: describe the format in the `to_dot()` Doxygen comment.
---
### C++20 Specifics
* **Modules** are **not** used – the project remains header‑only for maximum compatibility.
* **Concepts** may be introduced later for `State`/`Event` constraints, but currently we rely on `static_assert` with `std::is_enum_v` where appropriate.
* **`[[likely]]` / `[[unlikely]]`** may be added to the dispatch lookup for optimisation.
---
## Repository‑Specific Rules
* **Commit messages** should follow the **Conventional Commits** format (e.g., `docs: update README with link`, `feat(runtime): add guard support`). This ensures consistent changelog generation and clear history.

* **`.clang-format`** is the single source of truth for formatting; do not diverge.
* **No `.cursor` or Copilot instruction files** exist – agents should rely on the guidelines above.
* **All new public headers must be added to `include/fsm/`** and listed in the install target (`CMakeLists.txt`).
* **Tests must be placed under `test/`** and use Catch2’s `TEST_CASE` macro with a descriptive name.
* **CI should enforce:**
  1. `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
  2. `cmake --build build`
  3. `ctest --test-dir build --output-on-failure`
  4. `clang-format -output-replacements-xml …` (fail on any replacement).
---
## Running a Single Test
Catch2 provides a built‑in filter:
```bash
# Replace <case> with the exact test case name (case‑sensitive)
./build/fsm_tests --test-case "dot generation"
```
Alternatively, use CTest’s pattern matching:
```bash
ctest --test-dir build -R fsm_tests --output-on-failure
```
Both commands run **only** the specified test case and report failures instantly.
---
## Helpful Scripts & Aliases
Add the following to your `~/.bashrc` or project‑local `scripts/` folder:
```bash
# Alias for a one‑liner build‑test cycle
alias fsm-build='cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build && ctest --test-dir build --output-on-failure'

# Reformat the whole project quickly
alias fsm-fmt='clang-format -i $(git ls-files "*.hpp" "*.cpp")'
```
These shortcuts speed up day‑to‑day development and keep the repository clean.
---
*End of AGENTS.md – agents should read this file to understand how to build, test, lint, and style the code.*