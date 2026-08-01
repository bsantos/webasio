# Copilot instructions for webasio

## Reading and exploring source

> **MANDATORY FIRST ACTION, every session:** before any other tool call, run
> `tool_search` with query `codegraph` to load `codegraph_explore`. Do this even
> if the task looks like it only touches build/config files — you almost always
> end up reading code. Loading it up front removes the excuse to skip it later.

**HARD GATE — read this as a rule on `grep_search` / `read_file` / `file_search`,
not on codegraph.** Before you call `grep_search`, `read_file`, `file_search`, or
terminal `grep`/`cat`/`find` on ANY path under `inc/`, `src/`, `tests/`,
`example/`, or `deps/`, STOP. Those calls are FORBIDDEN as a first look at code.
You MUST call `codegraph_explore` first. If you catch yourself typing one of these
tool names against a code path, that is the signal you skipped codegraph — go back.

Ask yourself before every file read: *"Is this indexed source? If yes, did I query
codegraph first?"* If you can't answer yes to the second, you are about to repeat
the mistake.

- `codegraph_explore` returns verbatim, line-numbered source plus call paths and
  blast radius in one call — treat what it shows as already read; do NOT re-open
  those files. It is faster and more accurate than a grep/read loop. Use it before
  editing, not just for questions.
- `grep_search` / `read_file` / `file_search` / terminal `grep`/`cat` are allowed
  ONLY for: (1) confirming a specific detail codegraph already surfaced, or
  (2) content codegraph does NOT parse — `.ipp` implementation files (Asio's
  `impl/*.ipp` are only tracked as `#include` edges, so symbols defined inside
  them are NOT in the graph — read those directly), plus configs, docs, and
  build files (`CMakeLists.txt`, `.md`, `.yml`, etc.).
- If codegraph reports a project isn't indexed (no `.codegraph/`), stop using it
  for that project this session and use the built-in tools instead.
- After editing, check the codegraph staleness banner: files listed as edited
  since the last index sync should be re-read directly for accurate content.
