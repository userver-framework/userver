# userver — Agent Guide

## Quality requirements

Read and follow [the code review rules](scripts/docs/en/userver/development/code_review.md)
when implementing changes and reviewing PRs.

Before reporting implementation work as complete, review the entire diff
against these rules, fix introduced violations, and rerun relevant checks.

## Scope and version control

- Do **not** update changelogs unless explicitly asked.
- Do **not** add agent attribution: no `Generated with ...` lines in PR descriptions and no `Co-Authored-By: <agent>` trailers in commit messages.

## Project layout

Usage and testing examples: `samples/`. Documentation pages: `scripts/docs/en/`.

## Build systems

The project is built with both **CMake** and the internal **ya.make** build system. Always update `CMakeLists.txt`. Update `ya.make` **only if it already exists** for the affected target; otherwise change only CMake.

**Always** ask for explicit user permission before forcibly deleting build caches (e.g. wiping build directories, `ya`/`cmake` caches, or similar). Do not clear them on your own.

## CMake

Read [the CMake guide](ai/rules/cmake-guide.md) before any CMake changes.
