# Copilot instructions for webasio

## Reading and exploring source

- Prefer the codegraph MCP tool (`codegraph_explore`) for reading and
  understanding indexed source in this workspace. It returns verbatim,
  line-numbered source plus call paths and blast radius in a single call,
  which is faster and more accurate than a manual grep/read loop.
- Reach for `codegraph_explore` before falling back to `grep_search`,
  `read_file`, or terminal `grep`/`cat`. Use those lower-level tools only to
  confirm a specific detail codegraph didn't cover, or for content codegraph
  doesn't index (configs, docs, build files).
- After editing, check the codegraph staleness banner: files listed as edited
  since the last index sync should be re-read directly for accurate content.
