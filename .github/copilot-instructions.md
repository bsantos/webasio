# Copilot instructions for webasio

## Reading and exploring source

- ALWAYS use the codegraph MCP tool (`codegraph_explore`) FIRST when reading or
  understanding indexed source. The index covers this workspace's own code
  (`inc/`, `src/`, `tests/`, `example/`) AND the `deps/` tree, which are queryable
  just like workspace code. It returns verbatim, line-numbered source plus call
  paths and blast radius in a single call — faster and more accurate than a manual
  grep/read loop. Do this before editing, not just for questions.
- `codegraph_explore` is a deferred tool: load it once per session with
  `tool_search` (e.g. query "codegraph") BEFORE the first call. Do not skip
  straight to `grep_search`/`read_file` because loading feels like a detour.
- Fall back to `grep_search`, `read_file`, or terminal `grep`/`cat` only for:
  (1) confirming a specific detail codegraph didn't cover, or (2) content
  codegraph does NOT parse — `.ipp` implementation files (Asio's `impl/*.ipp`
  are only tracked as `#include` edges, so symbols defined inside them are NOT
  in the graph — read those directly), plus configs, docs, and build files.
- After editing, check the codegraph staleness banner: files listed as edited
  since the last index sync should be re-read directly for accurate content.
