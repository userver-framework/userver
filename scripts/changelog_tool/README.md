# Changelog Tool

A tool for automatically generating changelogs from git commits using LLM analysis.

## Features

- **Two-part LLM classification**: a main pass classifies every commit (feature, breaking-change,
  optimization, refactor, minor, docs, unclear) — no content heuristics — but only
  breaking-change/feature/optimization are changelog-eligible. A second, **creative** pass then
  runs over external commits the main pass didn't promote, bucketing them into Build /
  Documentation / Fixes / Other bottom sections (or promoting a genuine new feature into the main
  changelog instead).
- **Human overrides always win**: a commit with an explicit "Changelog entry"/"Changelog line" in
  its message, or one a reviewer force-includes via `refine`, always reaches the changelog even if
  the LLM classification isn't one of the three top-level categories — it renders in a dedicated
  `Forced` section instead of being silently dropped.
- **External contributor detection**: identifies external contributors and generates per-entry
  "Many thanks to ..." acknowledgments.
- **Component extraction**: extracts component names from commit titles for better organization.
- **Oversize commit handling**: commits too large for a single LLM prompt are isolated and analyzed
  without their diff (title/message/file list only), flagged `needs_attention`, and still make it
  into the changelog with a visible warning marker — nothing is silently dropped or truncated.
- **Reviewer workflow via `excluded.md`**: check a box to force-include a commit verbatim or trigger
  LLM re-analysis; no separate `override.yaml`. External leftovers are pre-filled with the creative
  pass's suggested line.
- **Breaking-change migration guides**: breaking changes carry both a `changelog_line` (what changed)
  and a separate `migration_guide` (how to adapt), rendered as two parts in the changelog.
- **Doxygen-style code references**: the LLM is instructed to format all code references in
  `changelog_line` and `migration_guide` using Doxygen syntax — `@ref` for userver entities and
  files (e.g. `@ref userver::components::ComponentBase`, `@ref userver/core/component.hpp`),
  backticks for inline identifiers (e.g. `config.yaml`, `some_variable`), and triple backticks for
  multi-line code blocks. Markdown links `[text](url)` are also acceptable for external resources.
- **Reliability**: per-commit state persistence (separate state files for the main and external
  passes), retryable errors, an internal resume loop, and request-level backoff (honoring
  `Retry-After`, jitter, capped backoff) make a single invocation survive transient LLM failures
  and run to completion autonomously.

## Installation

1. Ensure you have Python 3.8+ installed
2. That's it — no manual dependency installation is needed. The `changelog-tool` entry point
   is self-bootstrapping: on first run it automatically creates a `.venv` directory next to the
   script, installs dependencies from `requirements.txt` into it, and re-executes itself using
   that venv's interpreter. On subsequent runs it detects the existing `.venv` and reuses it, so
   you never need to run `python3 -m venv` or `pip install` (or `source .venv/bin/activate`)
   yourself.

3. Set up environment variables:
```bash
export CHANGELOG_LLM_URL="https://your-llm-api.com/v1"
export CHANGELOG_LLM_API_KEY="your-api-key"      # Bearer {key} auth
# OR
export CHANGELOG_LLM_OAUTH_KEY="your-oauth-token" # OAuth {token} auth
export CHANGELOG_LLM_MODEL="your-model-name"
```

| Variable | Description |
|----------|-------------|
| `CHANGELOG_LLM_URL` | API base URL (required). The client appends `/chat/completions`. |
| `CHANGELOG_LLM_API_KEY` | Bearer API key. Sent as `Authorization: Bearer {key}`. |
| `CHANGELOG_LLM_OAUTH_KEY` | OAuth token (alternative to the API key). Sent as `Authorization: Oauth {token}`. Some providers use this instead of a Bearer key. |
| `CHANGELOG_LLM_MODEL` | Model name (required). |

## Configuration

The tool is configured via `changelog.yaml`:

```yaml
generate:
  from_sha: <commit-sha>  # Starting commit SHA
  to_sha: HEAD           # Ending commit SHA (default: HEAD)
  repo_path: ../..       # Path to the repository (default: ../..)
  core_team_patterns:    # Patterns to identify core team members
    - ".*@userver\\.tech"
    - ".*@yandex-team\\.com"

llm-config:
  target_rps: 1                    # Target requests per second
  retries: 3                       # Number of retry attempts per request
  max_commits_per_batch: 10        # Maximum commits per LLM batch
  max_user_prompt_length: 100000   # Maximum prompt length in characters
  max_concurrent_requests: 5       # Maximum concurrent requests

  # Reliability knobs:
  max_rounds: 5                    # Max resume-loop rounds per invocation
  round_delay_seconds: 300         # Sleep between rounds while commits remain pending
  backoff_max_seconds: 60          # Cap for per-request exponential backoff

render:
  github_url: "https://github.com/userver-framework/userver"
```

## Usage

The tool runs in two passes: `generate` (fully automated) and `refine` (acts on your review).

### Step 1: Generate

```bash
./changelog-tool generate
```

This single, autonomous invocation:
1. Fetches commits from the specified range.
2. Sends every commit to the LLM for classification (no heuristic pre-filtering).
3. Internally retries commits that hit transient errors (429/5xx/network) across multiple rounds,
   with a delay between rounds, until they succeed or `max_rounds` is reached.
4. Writes `.changelog/classified.json`.
5. Renders `.changelog/changelog.md` (including oversize commits with a `⚠ analyzed without diff`
   marker) and `.changelog/excluded.md` (everything not going into the changelog, plus a distinct
   "Unprocessed / failed" section for commits that never completed).

It prints a summary line and exits with:
- `0` — everything processed successfully;
- `2` — partial success (some commits still failed after `max_rounds`); safe to re-invoke, or to
  proceed straight to reviewing/refining what did succeed;
- `1` — hard failure (e.g. git error, missing config, missing env vars).

### Step 2: Review `excluded.md`

Open `.changelog/excluded.md`. Each excluded commit is a GitHub task-list item:

```
- [ ] [`abc12345`](https://github.com/org/repo/commit/abc123...) commit title
      classification: minor
      changelog_line:
      analysis: ...
```

To act on a commit, **check its box**, then:
- **leave/edit `changelog_line`** with text → the commit will be **force-included verbatim** on the
  next `refine` run (no LLM call, your text is used as-is);
- **leave `changelog_line` empty** → the commit will be **re-analyzed by the LLM** on the next
  `refine` run.

Unchecked items are left alone.

### Step 3: Refine

```bash
./changelog-tool refine
```

This reads your checked items from `excluded.md`, applies the force-include / re-analyze actions
(re-analysis uses the same internal resume loop as `generate`), re-writes `classified.json`, and
re-renders both `changelog.md` and `excluded.md`. Promoted commits disappear from `excluded.md`.
Same exit-code semantics as `generate`. You can run `refine` again after checking more boxes.

## Output Format

The generated changelog has the following structure. The top part contains only three
changelog-eligible categories from the main pass (`refactor`/`minor`/`docs`/`unclear` never
appear here, however classified); a `Forced` catch-all section follows for human-overridden
commits the LLM didn't classify into one of those three; the bottom part contains the four
groups produced by the creative external-contribution pass (Part B) for external commits the
main pass did not promote:

```markdown
Breaking Change:

  * component1
    * changelog line 1 <!-- abc12345 -->
      * Migration: how to adapt to this breaking change
  * changelog line without component <!-- ghi12345 -->
    * Migration: ...

Features:

  * component1
    * changelog line 3 <!-- jkl67890 -->
  * changelog line without component <!-- mno12345 -->
  * big feature analyzed without a diff <!-- stu67890 --> ⚠ analyzed without diff — verify
  * a genuine new feature found by the external pass Many thanks to External Contributor for the PR! <!-- ext12345 -->

Optimizations:

  * component2
    * changelog line 4 <!-- pqr12345 -->

Forced:

  * component4
    * a commit with an explicit "Changelog entry" the LLM filed under docs/minor/refactor/unclear <!-- vwx12345 -->
  * a commit a reviewer force-included via refine despite an unhelpful classification <!-- yza67890 -->

Build:

  * Upgraded a dependency. Many thanks to External Contributor 1 for the PR! <!-- def12345 -->

Documentation:

  * Fixed a doc typo. Many thanks to External Contributor 2 for the PR! <!-- ghi12345 -->

Fixes:

  * component3
    * Fixed a bug. Many thanks to External Contributor 3 for the PR! <!-- jkl12345 -->

Other:

  * Minor unclassified cleanup. Many thanks to External Contributor 4 for the PR! <!-- mno12345 -->
```

Breaking-change entries carry two distinct parts: `changelog_line` describes the change itself, and
a nested `Migration:` line carries the separate `migration_guide` produced by the LLM.

`Fixes` entries are grouped by `component`, like the main sections; `Build`/`Documentation`/`Other`
are flat. Every bottom-section entry ends with `Many thanks to <Name> for the PR!` (name only, no
GitHub link). A genuine new feature found by the creative pass is promoted straight into `Features`
instead of appearing at the bottom.
