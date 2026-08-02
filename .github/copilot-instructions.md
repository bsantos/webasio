# Copilot Agent Instructions

This document is the authoritative source of repository information. Do not search the repository for facts stated here.

## Repository
- `webasio`: C++20 ASIO utility library — coroutines, logging, memory cache. CMake >= 3.28, Ninja, clang, `-Wall -Wextra -Werror`.
- Layout: `inc/webasio` public headers (`asio/`, `coro/`, `coro/detail/`, `net/`), `src/` library sources, `tests/` Boost.Test suite, `example/` demo, `deps/boost` vendored Boost.
- Targets: `webasio` (library, links `Boost::asio`), `unit_tests` (`EXCLUDE_FROM_ALL`, run via the `check` target or `ctest`), `example`.

## Response Style
- Answer in the fewest words possible. One sentence per point. Drop articles, filler and hedging.
- Use bullets, tables and short code blocks. No prose paragraphs, greetings, intros, summaries or meta-commentary.
- Ask at most one question per reply.
- These limits apply to chat text only; code output stays complete and readable.

## Coding Style
- Match the style of the surrounding code; where none applies, use Boost C++ style.
- Use C++20 features when they remove boilerplate or shorten the code.
- Include groups, in this order, one blank line between groups, alphabetical within each group: webasio, C++ third-party (Boost), standard library, system and C third-party. In a `.cpp`, its own header comes first, alone.
- RAII only: no raw `new`/`delete`; `std::unique_ptr` unless shared ownership is required.
- Mark `const` everything that is never mutated.
- Prefer the standard library, then Boost, over new implementations. Exception: never use `<regex>`; use Boost.Regex or Boost.Spirit.
- Add no dependencies, abstractions or refactorings the task does not require. Preserve public header boundaries and existing interfaces.

## Performance
- Optimize by doing less work, not the same work faster.
- Avoid copies: pass by reference or move. Pass trivially copyable objects up to ~16 bytes by value, even read-only. When the callee stores a copy, take by value and move.
- When traversal is frequent, use contiguous containers (`std::vector`, `std::array`, `std::string`), not node-based ones (`std::list`, `std::map`, `std::unordered_map`).
- Use unordered containers unless ordering is required; use Boost.MultiIndex when multiple access patterns are required.

## Concurrency
- Boost.ASIO is the mandatory async I/O framework.
- Write new and refactored async code as coroutines with the webasio coro library, unless it's a generic library using ASIO async state machines. Mix callbacks and coroutines only when an existing API forces it.
- Never block inside a coroutine or an ASIO thread.
- Serialize with executors instead of mutexes; never guard allocating or deallocating data structures with a mutex.

## Error Handling
- Follow the propagation pattern already used in the file being edited.
- Expected or frequent errors: return error codes or `std::optional`. Reserve exceptions for rare, unexpected failures.
- Bugs: fail fast with `webasio::assert_trap()`, never exceptions.
- Log only through `webasio::logger`; do not alter existing logging behaviour.
- Keep config files, protocol messages and public interfaces backwards compatible unless the task states otherwise.

## Build & Test
- Unit tests use Boost.Test. Add tests for every new or refactored unit.
- Building and running tests need no permission. Before the first build of a session, ask the user once for the build directory to use, unless repository memory already records it; reuse that answer for the rest of the session. Never guess a build directory or re-configure one with invented `-D` values.
- A new `#include` of another library also needs its include dir and its library added to the build files, or the build breaks.
- `src/CMakeLists.txt` lists every header explicitly: a new header under `inc/webasio` must be added there. A new Boost library must be added to `BOOST_INCLUDE_LIBRARIES` in the top-level `CMakeLists.txt`.

## Code Exploration — `codegraph_explore`
Use `codegraph_explore` instead of `read_file` and grep/read loops for C/C++ code; follow the tool's own usage instructions, plus:

- Deferred tool: load it once per session with `tool_search` "codegraph" before the first call, or it is not callable.
- One index at the workspace root covers every C/C++ source and header not excluded by `.gitignore`, `deps/boost` included.
- Not indexed — use `grep_search`/`read_file`: `CMakeLists.txt`, `*.cmake`, `codegraph.json`, `.github/**`, `*.md`.
- Call it before reading files, for questions and edits alike; one call usually suffices.
- Trust its output — it comes from a full AST parse. Do not re-verify with grep. Treat returned source as already read.
- Ambiguous cross-file calls may return multiple candidates. It validates nothing; the compiler, tests and linter remain authoritative.
- The index lags writes by ~1s. On a staleness banner, do other work first, else run `codegraph sync`.
- If it reports the project is not indexed, stop calling it and use the built-in search tools.
