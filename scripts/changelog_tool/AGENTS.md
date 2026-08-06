# Changelog Tool

This agent is responsible for running the changelog tool, which collects commit information,
classifies commits via LLM, identifies external contributors, and renders a changelog for review.

## Two-part classification

**Part A — main pass.** Every commit is classified by the LLM — there are no content heuristics
(no size thresholds, no keyword-based pre-filtering). The LLM returns one of: `feature`,
`breaking-change`, `optimization`, `refactor`, `minor`, `docs`, `unclear`. Only the first three
(`breaking-change`, `feature`, `optimization`) are changelog-eligible: `to_changelog=true` is
never valid for `refactor`/`minor`/`docs`/`unclear`, and the renderer's main-section order constant
only contains those three anyway (defense in depth, both enforced independently). Per-category
prompt guidance: `breaking-change` produces a separate `migration_guide` field (see below);
`feature` describes the component and what was done; `optimization` focuses on concrete numbers
(%, ns, bytes, counts) when the commit provides them, describing what/why/measured-effect, without
inventing figures when none are given.

**Part B — creative external pass.** External commits the main pass did NOT promote to the
changelog ("external leftovers": `is_external` and not `(to_changelog is True and changelog_line)`)
are sent to a second, separate LLM prompt (`EXTERNAL_CONTRIB_SYSTEM_PROMPT`), diffless (title +
message + changed-file list only, no diff, no oversize handling needed). For each one the LLM
writes a short `changelog_line` and assigns a `group`: `Build` / `Documentation` / `Fixes` /
`Other`. A genuine new feature is instead promoted (`promote_to_feature=true`) straight into the
main changelog's Features section, with `to_changelog=True` and `classification=feature` set.
`Fixes` entries always carry a `component`, like main-pass entries. Results are persisted in a
**separate** state file `external_state.json` (never `llm_state.json`) so a SHA processed by both
passes never has one result clobber the other.

## Human overrides never get silently dropped: the `Forced` section

Two distinct mechanisms let a human insist a commit must reach the changelog regardless of what
the LLM thinks:
1. A "Changelog entry"/"Changelog line" section verbatim in the commit message (main-pass prompt
   override, see above) — forces `to_changelog=True`, but the LLM still independently picks a
   `classification`, which may land on `refactor`/`minor`/`docs`/`unclear`.
2. A reviewer checking an `excluded.md` item with a non-empty `changelog_line` during `refine`
   (force-include, no LLM call) — this only sets `to_changelog`/`changelog_line`; the commit's
   existing `classification` (whatever the main pass assigned it) is left untouched.

In both cases the commit ends up with `to_changelog=True` and a non-empty `changelog_line`, but
its `classification` may not be one of the three main-section categories. Rather than let the A1
restriction silently drop such a commit, [`render_changelog()`](changelog_tool/render/changelog.py:54)
detects this exact combination and renders it in a dedicated `Forced:` catch-all section
(grouped by `component`, same shape as the main sections), positioned right after Optimizations
and before the external Build/Documentation/Fixes/Other sections. A human decision to include a
commit is never lost, independent of how the LLM classified it.

## Oversize commits and `needs_attention`

A commit whose prompt (including its diff) doesn't fit the configured `max_user_prompt_length`
even in an otherwise-empty batch is isolated into its own single-commit batch and re-analyzed
**without its diff** (title/message/changed-file-list only). Such commits are flagged
`needs_attention=true` and, if included in the changelog, get a visible
`⚠ analyzed without diff — verify` marker. Nothing is silently dropped or truncated.

## Breaking changes: two distinct fields

For `breaking-change` commits the LLM produces two separate fields:
- `changelog_line`: describes the breaking change itself;
- `migration_guide`: a separate, actionable recipe for how to fix/migrate.

Both are rendered in `changelog.md` as two parts: the `changelog_line`, followed by a nested
`Migration:` sub-line carrying the `migration_guide`.

## Doxygen-style code references in `changelog_line` and `migration_guide`

Both the main pass ([`SYSTEM_PROMPT`](changelog_tool/llm/prompts.py)) and the external pass
([`EXTERNAL_CONTRIB_SYSTEM_PROMPT`](changelog_tool/llm/prompts.py)) instruct the LLM to format all
code references in `changelog_line` (and `migration_guide` for the main pass) using Doxygen syntax:

- **userver code entities** (classes, functions, methods, namespaces, enumerations, macros, etc.):
  `@ref` with the fully-qualified name, e.g. `@ref userver::components::ComponentBase`.
- **Files and directories**: `@ref` with the path, e.g. `@ref userver/core/component.hpp`.
- **Inline code fragments** (identifiers, variables, parameters, config values): backticks, e.g.
  `config.yaml`, `max_concurrent_requests`.
- **Multi-line code blocks**: triple backticks with the language.
- **Markdown links** `[text](url)` and HTML tags are also acceptable when more natural than a
  Doxygen command (e.g. linking to an external resource).

The renderer ([`render_changelog()`](changelog_tool/render/changelog.py:35)) outputs these fields
verbatim, so the Doxygen markup flows straight into the generated `changelog.md`.

## `excluded.md` and the `refine` workflow

`excluded.md` replaces the old `review_report.md` + `override.yaml` pair. It's a GitHub task list;
each excluded commit is one checkbox item:

```
- [ ] [`abc12345`](<github_url>/commit/<full-sha>) commit title
      classification: minor
      changelog_line:
      analysis: ...
```

**The checkbox is the reviewer's decision.** Checking an item tells `refine` to act on it:
- `changelog_line` non-empty (as-is or edited) → **force-include verbatim**: `to_changelog=true`,
  the reviewer's text is used as-is, **no LLM call**.
- `changelog_line` empty → **re-analyze**: the commit's cached LLM state is dropped and it is
  re-run through the LLM (via the same resume loop `generate` uses).

For external leftovers, `changelog_line` is **pre-filled with the creative pass's suggestion**
(Part B), so unchecked items already show a proposed line the reviewer can just approve by
checking the box (force-include, no LLM call) rather than typing one from scratch. `refine` never
re-runs the creative pass itself — it only re-renders the four bottom sections (or the aggregate
`* Many thanks to:` fallback) from what's already in `classified.json`; force-including an item
clears its `external_group` so it doesn't linger in both a main section and a bottom section.

Unchecked items stay excluded. `refine` re-renders both `changelog.md` and `excluded.md`; promoted
commits leave `excluded.md`.

Oversize (`needs_attention`) commits go straight into `changelog.md` with a visible warning marker
on the first `generate` run — they do **not** go into `excluded.md` first.

## Reliability & autonomous operation

A single `generate` (or `refine`) invocation drives itself to completion despite transient LLM
errors, and is safe to interrupt and re-run:

- **State persistence**: `llm_state.json` checkpoints per-commit, atomically (tempfile +
  `os.replace`). A completed commit is never re-billed to the LLM on a later round or run. Errored
  commits stay retryable.
- **Request-level resilience**: on `429`, the client honors a `Retry-After` header when present
  (falling back to exponential backoff otherwise), adds jitter, and caps the delay at
  `backoff_max_seconds`. Fatal-vs-transient status codes are unchanged: `400/401/403/404` fail
  fast; `429/5xx/network` errors retry.
- **Run-level resume loop**: after processing, the command checks for commits still
  pending/errored and, if any remain and progress was made, sleeps `round_delay_seconds` and
  retries, up to `max_rounds`. It stops early if a full round makes zero progress (guards against
  infinite loops).
- **Partial-success semantics**: the run always writes `changelog.md` + `excluded.md` from
  whatever succeeded. Commits still failed after `max_rounds` appear in a distinct
  "Unprocessed / failed — needs retry" section of `excluded.md` with their last error.
- **Exit codes**: `0` — all processed; `2` — partial (some still failed; safe to re-invoke);
  `1` — hard failure (missing env vars, git error, config error). A summary line is printed:
  `processed=X failed=Y needs_attention=Z rounds=R`.

### Config knobs (`llm-config` in `changelog.yaml`)

`max_rounds` (default 5), `round_delay_seconds` (default 300), `backoff_max_seconds` (default 60),
plus the existing `target_rps`, `max_concurrent_requests`, `retries`.

## Usage

The `changelog-tool` entry point is self-bootstrapping: it automatically creates a `.venv`
(installing dependencies from `requirements.txt` on first run) and re-executes itself inside
that virtual environment if it is not already running there. You do NOT need to manually
`source .venv/bin/activate` before running it — simply invoke it directly:

```bash
./changelog-tool [command] [options]
```

## Commands

### generate

Fetches commits from the specified range, sends all of them to the LLM for classification
(wrapped in the internal resume loop described above), and renders `changelog.md` + `excluded.md`.

```bash
./changelog-tool generate [options]
```

Options:
- `--from-sha`: Starting commit SHA (overrides config)
- `--to-sha`: Ending commit SHA (overrides config)
- `--repo-path`: Path to the repository (overrides config)

Exit code: `0` / `2` / `1` (see Reliability section above).

### refine

Acts on the reviewer's checked items in `excluded.md` (force-include verbatim, or re-analyze via
the LLM), then re-renders both `changelog.md` and `excluded.md`.

```bash
./changelog-tool refine
```

Exit code: `0` / `2` / `1`, same semantics as `generate`.

## Output Directory

By default, the tool writes to `.changelog/` (`classified.json`, `llm_state.json`, `changelog.md`,
`excluded.md`). You can customize this with the `--output-dir` global option:

```bash
# Run with custom output directory
./changelog-tool --output-dir ./my-output-dir generate
./changelog-tool --output-dir ./my-output-dir refine
```
