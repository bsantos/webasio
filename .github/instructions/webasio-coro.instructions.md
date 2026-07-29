---
description: "Guidelines for implementing and modifying webasio::coro APIs, await flows, cancellation, executor hopping, and coroutine tests. Use when working on coroutine headers/tests or suggesting coroutine-based examples."
applyTo: "inc/webasio/coro/**/*.hpp,tests/coro/**/*.cpp,example/**/*.cpp,example/**/*.hpp"
---

# webasio::coro Copilot Guidelines

## Scope and intent

- Keep changes aligned with existing webasio::coro semantics and test behavior.
- Preserve compatibility with Boost.Asio completion model and cancellation model.
- Prefer minimal, local edits that do not alter public APIs unless explicitly requested.

## Preferred public types and APIs

- Use `webasio::co_promise<Ts...>` or `webasio::coro::promise<Ts...>` for coroutine return values that are awaited.
- Use `webasio::co_detached` or `webasio::coro::detached` for fire-and-forget coroutines.
- Use `webasio::coro::await(...)` for callback/deferred integration when adapting a coroutine into an Asio completion token flow.
- Use `webasio::coro::outcome<Ts...>` values from await callbacks and extract values with `get()`.

## Await and error propagation rules

- Prefer `co_await` directly on Asio async operations where supported.
- Keep exception behavior consistent with tests:
  - `promise<T...>` exceptions propagate to the caller on `co_await`.
  - callback-style `await(...)` surfaces exceptions through `outcome::get()`.
- Do not swallow exceptions silently; catch only at intended boundaries (for example, top-level detached workflows that convert errors into state).

## Cancellation rules

- When a coroutine needs cancellation awareness, establish cancellation state early with:
  - `co_await webasio::coro::reset_cancellation_state(slot)`
  - or the appropriate overload with filters.
- Read cancellation status with `co_await webasio::coro::cancelled`.
- Read or set slot via `webasio::coro::cancellation_slot` and `reset_cancellation_state(...)` helpers.
- For fan-out cancellation scenarios, prefer `webasio::coro::multicast_cancellation` and per-task `multicast_cancellation_signal`.

## Executor and scheduling rules

- Use `webasio::coro::dispatch(executor)` when continuation should run inline if possible.
- Use `webasio::coro::post(executor)` when continuation must be queued.
- Keep thread-affinity behavior explicit in tests and examples when hopping across executors.

## Style and structure

- Match existing style in this repository:
  - `#pragma once` in headers.
  - namespace style `namespace webasio::coro { ... }`.
  - concise types and aliases, avoid over-abstracting small awaitable adapters.
- Keep coroutine helpers deterministic and cheap; avoid introducing blocking calls inside coroutine bodies.
- Avoid introducing alternative async frameworks; stay within Boost.Asio and existing webasio abstractions.

## Testing expectations

- Add or update tests in `tests/coro/` for behavioral changes.
- For cancellation-related changes, verify both cancelled and non-cancelled flows.
- For executor-related changes, verify expected execution context/thread where relevant.
- Keep tests in Boost.Test style used by the repository.

## Review checklist (for PRs and code reviews)

- `promise<T...>` still propagates exceptions on `co_await`.
- `detached` flows only catch exceptions at intentional boundaries.
- `await(...)` callback signatures remain aligned with `promise_outcome_t`.
- Cancellation-aware coroutines initialize state with `reset_cancellation_state(...)` before awaiting cancellable work.
- Cancellation assertions validate both operation outcome (`operation_aborted` or success) and `co_await cancelled` state.
- `dispatch` versus `post` usage matches required scheduling semantics.
- Cross-executor code keeps thread-affinity expectations test-covered.
- Public API changes in `inc/webasio/coro/` are paired with test updates in `tests/coro/`.

## Suggestion quality bar for Copilot

- Favor concrete snippets that compile within this codebase over generic coroutine examples.
- Reuse existing helpers (`sleep_for`, `await`, `dispatch`, `post`, cancellation utilities) before proposing new primitives.
- If a suggestion changes semantics, include a test change in the same proposal.