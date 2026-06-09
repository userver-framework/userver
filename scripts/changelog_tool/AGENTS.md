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

## Output Directory

By default, the tool outputs classified commits to `.changelog/preclassified.json`. You can customize this with the `--output-dir` global option:

```bash
# Run with custom output directory
./changelog-tool --output-dir ./my-output-dir collect
```