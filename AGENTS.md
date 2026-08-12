# userver — Agent Guide

## Generic rules

- DRY
- defensive programming, no implicit contracts
- informative configuration error message, include 'how to fix' hints if possible
- avoid no-op code, it might be misleading
- Do **not** update changelogs unless explicitly asked

## Comments and readable code

- Prefer readable code over comments: extract magic numbers/strings into `constexpr` (or similarly named) constants with clear names
- Where the code is still unclear without a comment, add a **short** comment that is easy to understand
- code comments must describe "why" and "what", not "how"
- avoid trivial comments
- use comments to explicitly mark tricky/buggy/TODO code

## Language and practices

- Use modern **C++20** and established best practices of the project.
- Check invariants with `UASSERT` / `UASSERT_MSG` (`userver/utils/assert.hpp`).
- Prefer code that is simple to read and reason about.
- Prefer clear ownership:
  - `std::unique_ptr` over `std::shared_ptr` when shared ownership is not required;
  - `utils::FastPimpl` when hiding implementation details;
  - avoid storing raw pointers or references as class members when values, `unique_ptr`, or indices/handles suffice.
- Do not rely on transitive includes; include what you use.
- Public API lives under `*/include/userver/...` and is included as `#include <userver/...>`. Treat `impl` / `detail` as implementation details.

## Synchronization and blocking

Do not use C++ standard library / libc synchronization or blocking IO on the main task processor. Use userver primitives instead (see `scripts/docs/en/userver/intro.md` and `scripts/docs/en/userver/synchronization.md`):

## Tests and documentation

Any new functionality or bug fix **must** be covered by tests. Add or update documentation when it makes sense.

If asked to fix a flaky failure and the root cause is in production code (not in the test itself), **first** add a new test that reliably reproduces the bug without flakiness, then fix the code.

Usage and testing examples: `samples/`. Documentation pages: `scripts/docs/en/`.

## Build systems

The project is built with both **CMake** and the internal **ya.make** build system. Always update `CMakeLists.txt`. Update `ya.make` **only if it already exists** for the affected target; otherwise change only CMake.

**Always** ask for explicit user permission before forcibly deleting build caches (e.g. wiping build directories, `ya`/`cmake` caches, or similar). Do not clear them on your own.

## CMake

`ai/rules/cmake-guide.md` - guide for cmake. Read it before any cmake changes.
