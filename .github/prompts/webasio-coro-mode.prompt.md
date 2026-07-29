---
description: "Router prompt for webasio::coro tasks. Use to choose AUTHOR mode for implementation work or REVIEW mode for PR/diff analysis."
---

# webasio::coro Mode Router

Use this prompt to start a coroutine task with explicit mode selection.

## Inputs

- MODE: `author` or `review`
- SCOPE: files, symbols, or module area (for example `inc/webasio/coro/await.hpp`, `tests/coro/cancellation.cpp`)
- TASK: what you want done
- CONSTRAINTS: optional constraints (no API changes, no new deps, etc.)

## Prompt template

MODE: <author|review>
SCOPE: <paths or symbols>
TASK: <request>
CONSTRAINTS: <optional>

For MODE `author`:
- Use agent `webasio-coro-author`.
- Implement minimal changes aligned with existing coroutine semantics.
- Preserve public APIs unless explicitly requested.
- Update tests in `tests/coro/` for behavior changes.

For MODE `review`:
- Use agent `webasio-coro-review`.
- Return findings first, ordered by severity, with file/line references.
- Focus on regressions in cancellation, await behavior, executor semantics, and coroutine lifetime safety.
- Explicitly call out missing tests.

## Example 1

MODE: author
SCOPE: inc/webasio/coro/cancellation.hpp, tests/coro/cancellation.cpp
TASK: Add helper for resetting cancellation state with explicit in/out filters.
CONSTRAINTS: Keep existing API behavior and add tests.

## Example 2

MODE: review
SCOPE: inc/webasio/coro/**/*.hpp, tests/coro/**/*.cpp
TASK: Review current branch changes for behavioral regressions.
CONSTRAINTS: Prioritize correctness over style.
