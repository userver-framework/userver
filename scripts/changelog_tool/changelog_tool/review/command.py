import pathlib
from typing import List

import changelog_tool.common.io as io
from changelog_tool.config import Config
from changelog_tool.collect.classification import ClassifiedCommit, Classification


def review(config: Config) -> None:
    print(f"Loading classified commits from {config.review.output_dir}...")
    classified_commits: List[ClassifiedCommit] = io.load_classified_commits(
        config.review.output_dir, 'classified.json'
    )
    
    if not classified_commits:
        print("No classified commits found. Please run 'collect' command first.")
        return
    
    print(f"Found {len(classified_commits)} classified commits")
    
    # Split commits into two groups
    not_in_changelog: List[ClassifiedCommit] = []
    in_changelog: List[ClassifiedCommit] = []
    
    for commit in classified_commits:
        if commit.to_changelog is False or commit.classification == Classification.UNCLEAR:
            not_in_changelog.append(commit)
        elif commit.to_changelog is True:
            in_changelog.append(commit)
    
    # Sort both groups by score_size (descending)
    not_in_changelog.sort(key=lambda c: c.score_size, reverse=True)
    in_changelog.sort(key=lambda c: c.score_size, reverse=True)
    
    # Generate markdown report
    markdown_content = _generate_markdown_report(
        not_in_changelog, in_changelog, config.review.github_url
    )
    
    # Generate override YAML
    override_yaml_content = _generate_override_yaml(
        not_in_changelog, in_changelog
    )
    
    # Write output files
    output_dir = config.review.output_dir
    output_dir.mkdir(parents=True, exist_ok=True)
    
    markdown_file = output_dir / 'review_report.md'
    with open(markdown_file, 'w') as f:
        f.write(markdown_content)
    print(f"Generated markdown report: {markdown_file}")
    
    override_file = output_dir / 'override.yaml'
    with open(override_file, 'w') as f:
        f.write(override_yaml_content)
    print(f"Generated override YAML: {override_file}")


def _generate_markdown_report(
    not_in_changelog: List[ClassifiedCommit],
    in_changelog: List[ClassifiedCommit],
    github_url: str
) -> str:
    lines = []
    
    # Header
    lines.append("# Changelog Review Report\n")
    
    # Not in changelog section
    lines.append("## Not in Changelog\n")
    lines.append(f"Total: {len(not_in_changelog)} commits\n")
    
    for commit in not_in_changelog:
        lines.append(_format_commit_markdown(commit, github_url))
        lines.append("")
    
    # In changelog section
    lines.append("## In Changelog\n")
    lines.append(f"Total: {len(in_changelog)} commits\n")
    
    for commit in in_changelog:
        lines.append(_format_commit_markdown(commit, github_url))
        lines.append("")
    
    return "\n".join(lines)


def _format_commit_markdown(commit: ClassifiedCommit, github_url: str) -> str:
    short_sha = commit.sha[:8]
    commit_url = f"{github_url}/commit/{commit.sha}"
    
    lines = []
    lines.append(f"### [{short_sha}]({commit_url}) {commit.title}")
    lines.append("")
    
    # Status
    if commit.to_changelog is True:
        status = "✅ In Changelog"
    elif commit.to_changelog is False:
        status = f"❌ Not in Changelog (Classification: {commit.classification})"
    else:
        status = f"❓ Unclear (Classification: {commit.classification})"
    
    lines.append(f"**Status:** {status}")
    lines.append(f"**Size:** {commit.score_size} lines changed")
    lines.append("")
    
    # Changelog line (if available)
    if commit.changelog_line:
        lines.append(f"**Changelog Line:** {commit.changelog_line}")
        lines.append("")
    
    # Analysis (if available)
    if commit.commit_analysis:
        lines.append("**Analysis:**")
        lines.append(commit.commit_analysis)
        lines.append("")
    
    return "\n".join(lines)


def _generate_override_yaml(
    not_in_changelog: List[ClassifiedCommit],
    in_changelog: List[ClassifiedCommit]
) -> str:
    lines = []
    
    # Header comment
    lines.append("# Override file for changelog classification")
    lines.append("# Uncomment and modify entries to override classification")
    lines.append("")
    
    # Process all commits in order
    all_commits = not_in_changelog + in_changelog
    
    for commit in all_commits:
        lines.append(f"# {commit.sha}:")
        lines.append(f"#   commit_title: \"{commit.title}\"")
        
        if commit.to_changelog is True:
            to_changelog = "true"
        elif commit.to_changelog is False:
            to_changelog = "false"
        else:
            to_changelog = "null"
        
        lines.append(f"#   to_changelog: {to_changelog}")
        
        if commit.changelog_line:
            lines.append(f"#   changelog_line: \"{commit.changelog_line}\"")
        else:
            lines.append(f"#   changelog_line: null")
        
        lines.append("")
    
    return "\n".join(lines)
