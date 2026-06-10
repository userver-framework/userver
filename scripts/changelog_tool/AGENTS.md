# Changelog Tool

This agent is responsible for running the changelog tool, which collects commit information and identifies external contributors.

## Heuristics for LLM Analysis

The tool uses heuristics to determine which commits should be sent to an LLM for changelog analysis:

We calculate a `score_size` metric as `lines_added + lines_deleted` for each commit.

The tool will NOT send commits to the LLM if they meet any of these criteria:
1. Any file path contains "docs/" or "documentation", OR commit title contains documentation keywords
2. Commit title contains fix/bug keywords AND the commit is small (score_size <= 20)
3. All commits with score_size <= 20

Documentation keywords: "doc", "docs", "documentation", "readme"
Fix/bug keywords: "fix", "bugfix", "bug", "patch", "repair", "correct", "resolve"

## Usage

IMPORTANT: The changelog tool must always be run with the virtual environment activated:

```bash
# Always activate the virtual environment first
source .vent/bin/activate

# Run the tool
./changelog-tool [command] [options]
```

## Commands

### collect

Collects commits from the specified range and classifies them using heuristics and LLM analysis.

```bash
./changelog-tool collect [options]
```

Options:
- `--from-sha`: Starting commit SHA (overrides config)
- `--to-sha`: Ending commit SHA (overrides config)
- `--repo-path`: Path to the repository (overrides config)

### review

Generates a markdown report and an override YAML file for reviewing classified commits.

```bash
./changelog-tool review
```

The review command generates two files in the output directory:
- `review_report.md`: A markdown report showing all commits, sorted by size, with their classification status, changelog lines, and analysis
- `override.yaml`: A commented YAML file containing all commits that can be uncommented and modified to override classifications

The report is divided into two sections:
1. **Not in Changelog**: Commits that are not included in the changelog (either filtered by heuristics or marked as unclear)
2. **In Changelog**: Commits that are included in the changelog

Each commit in the report shows:
- Commit hash with link to GitHub
- Commit title
- Status (✅ In Changelog, ❌ Not in Changelog, or ❓ Unclear)
- Size (number of lines changed)
- Changelog line (if available)
- Analysis (if available)

### report

Generates a formatted Markdown changelog based on the review output and applies user overrides.

```bash
./changelog-tool report
```

The report command performs the following steps:
1. Loads classified commits from `classified.json`
2. Applies overrides from `override.yaml` (if present)
3. Identifies commits marked for the changelog that lack changelog lines or analysis
4. Runs these commits through the LLM with 1.5x increased prompt size and diff truncation enabled
5. Generates a formatted Markdown changelog grouped by classification:
   - Breaking Changes
   - Features
   - Optimizations
   - Bug Fixes
   - Refactoring
   - Minor Changes
   - Documentation
6. Appends "Many thanks to [Name] for the PR!" for external contributors in the changelog
7. Appends a section at the end for external contributors not included in the changelog
8. Saves the generated changelog to `changelog.md` in the output directory

## Output Directory

By default, the tool outputs classified commits to `.changelog/preclassified.json`. You can customize this with the `--output-dir` global option:

```bash
# Run with custom output directory
./changelog-tool --output-dir ./my-output-dir collect
./changelog-tool --output-dir ./my-output-dir review
```