# Changelog Tool

A tool for automatically generating changelogs from git commits using LLM analysis.

## Features

- **Automatic commit classification**: Classifies commits into categories (feature, bug, optimization, refactor, minor, docs, unclear)
- **LLM-powered analysis**: Uses LLM to analyze commits and generate changelog entries
- **External contributor detection**: Identifies external contributors and generates acknowledgments
- **Component extraction**: Extracts component names from commit titles for better organization
- **Override support**: Allows manual override of classifications and changelog entries
- **State persistence**: Saves LLM analysis results to avoid reprocessing
- **Rate limiting**: Configurable rate limiting and concurrent request limits

## Installation

1. Ensure you have Python 3.8+ installed
2. Install dependencies:
```bash
cd scripts/changelog_tool
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip3 install -r requirements.txt
```

3. Set up environment variables:
```bash
export CHANGELOG_LLM_URL="https://your-llm-api.com/v1"
export CHANGELOG_LLM_API_KEY="your-api-key" 
export CHANGELOG_LLM_MODEL="your-model-name"
```

## Configuration

The tool is configured via `changelog.yaml`:

```yaml
collect:
  from_sha: <commit-sha>  # Starting commit SHA
  to_sha: HEAD           # Ending commit SHA (default: HEAD)
  repo_path: ../..       # Path to the repository (default: ../..)
  core_team_patterns:    # Patterns to identify core team members
    - ".*@userver\\.tech"
    - ".*@yandex-team\\.com"

llm-config:
  target_rps: 1                    # Target requests per second
  retries: 3                       # Number of retry attempts
  max_commits_per_batch: 10        # Maximum commits per LLM batch
  max_user_prompt_length: 100000   # Maximum prompt length in characters
  include_diff: true               # Include diff in LLM prompt
  truncate_diff: false             # Truncate diff if too long
  max_concurrent_requests: 5       # Maximum concurrent requests

review:
  github_url: "https://github.com/userver-framework/userver"

report:
  github_url: "https://github.com/userver-framework/userver"
```

## Usage

### Step 1: Collect Commits

Run the `collect` command to gather commits and analyze them:

```bash
source .venv/bin/activate
./changelog-tool collect
```

The tool will:
1. Fetch commits from the specified range
2. Classify commits using heuristics
3. Send unclear commits to LLM for analysis
4. Save results to `.changelog/classified.json`

**Important**: Run the `collect` command repeatedly until you see a message like:
```
Found 10 commits, 10 already processed, 0 to process via LLM
```

This ensures all commits have been processed by the LLM. The tool uses state persistence to avoid reprocessing commits, so running it multiple times is safe and recommended for reliability.

### Step 2: Review and Override

Run the `review` command to generate a review report:

```bash
./changelog-tool review
```

This generates two files in `.changelog/`:
- `review_report.md`: A markdown report showing all commits with their classification status
- `override.yaml`: A commented YAML file for overriding classifications

Review the report and uncomment/modify entries in `override.yaml` to override classifications:

```yaml
# Example override.yaml
commit_sha_1:
  to_changelog: true
  changelog_line: "Added support for async LLM processing"

commit_sha_2:
  to_changelog: false
  classification: "minor"
```

Feel free to leave classification or changelog_line empty LLM will handle it on the next step.

### Step 3: Generate Changelog

Run the `report` command to generate the final changelog:

```bash
./changelog-tool report
```

This will:
1. Load classified commits from `classified.json`
2. Apply overrides from `override.yaml`
3. Process commits needing LLM analysis with increased prompt size (1.5x) and diff truncation
4. Generate a formatted Markdown changelog grouped by classification and component
5. Save the changelog to `.changelog/changelog.md`

**Important**: Run the `report` command repeatedly until you see a message like:
```
Found 10 commits, 10 already processed, 0 to process via LLM
```

This ensures all commits that need LLM analysis have been processed.

## Output Format

The generated changelog has the following structure:

```markdown
* Breaking Change
  * component1
    * changelog line 1 <!-- abc12345 -->
    * changelog line 2 <!-- def67890 -->
  * changelog line without component <!-- ghi12345 -->

* Feature
  * component1
    * changelog line 3 <!-- jkl67890 -->
  * changelog line without component <!-- mno12345 -->

* Optimization
  * component2
    * changelog line 4 <!-- pqr12345 -->

* Bug
  * component1
    * changelog line 5 <!-- stu67890 -->

* Refactor
  * component3
    * changelog line 6 <!-- vwx12345 -->

* Minor
  * changelog line 7 <!-- yza67890 -->

* Documentation
  * changelog line 8 <!-- bcd12345 -->

* Many thanks to:
  * External Contributor 1 for commit title 1!
  * External Contributor 2 for:
    * commit title 2
    * commit title 3
```