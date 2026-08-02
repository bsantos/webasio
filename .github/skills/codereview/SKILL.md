---
name: codereview
description: 'Review C++ changes — a commit, a diff, a branch or attached files. Use when the user asks for a code review, "review this commit/diff/PR/change", feedback on a patch, or a correctness/security/performance check before pushing. Explores blast radius with codegraph, and emits a Summary / Findings / Verdict report.'
---

# Code Review

Run steps 1-3 in order; skip none. Step 1 precedes any critique of the code. Modify no files unless the user asks for fixes.

Determine what to review: attached context or an explicitly named commit/branch/file; otherwise default to the current commit (`git show HEAD`).

If the reviewed commit is not an ancestor of `HEAD` (`git merge-base --is-ancestor <sha> HEAD`), codegraph reflects the working tree, not that commit — extract the commit's files (`git show <sha>:<path> > "$TMPDIR/..."`) and read those, using codegraph only for callers and blast radius.

## Step 1 — Explore

Load codegraph once with `tool_search` "codegraph" (deferred tool), then explore per `.github/copilot-instructions.md`: name every symbol the diff touches in one call — source, callers, call paths, blast radius, untested-symbol flags.

If the diff adds a source file, an include of another library, or a new target, check `CMakeLists.txt` with `read_file` (not indexed): are the new sources listed, and the new library's include dir and link entry added?

Collect diagnostics for the touched files with `get_errors`.

## Step 2 — Review

- Correctness: logic errors, off-by-one, wrong conditions, edge cases, null/error handling, races, resource leaks.
- Security: OWASP Top 10 — injection, unvalidated input, authn/authz gaps, secrets in code, unsafe deserialization, memory safety.
- Performance: needless allocations/copies, N+1 queries, blocking calls on coroutine/ASIO paths, algorithmic complexity, scalability.
- Design: consistency with surrounding patterns, separation of concerns, naming, dead/duplicated code, plus the coding, concurrency and error-handling rules in `.github/copilot-instructions.md`.
- Tests: Boost.Test coverage, missing edge cases, existing tests still pass.
- Docs: public APIs, comments and docs needing updates.

## Step 3 — Report

1. **Summary** — one paragraph: what the commit does.
2. **Findings** — ordered Critical, Major, Minor, Nit: file and line, issue and why it matters, concrete fix (snippet when it clarifies).
3. **Verdict** — Approve, Approve with comments, or Request changes, with a one-line justification.
