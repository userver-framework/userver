"""Render excluded.md: reviewer-editable task list of excluded commits."""

from changelog_tool.common.models import ClassifiedCommit

_UNPROCESSED_SECTION_TITLE = '## Unprocessed / failed \u2014 needs retry'


def render_excluded(
    commits: list[ClassifiedCommit],
    failed: dict[str, str],
    github_url: str,
) -> str:
    lines: list[str] = []

    lines.append('# Excluded Commits\n')
    lines.append(
        'Check a box and either leave `changelog_line` as-is / edit it to force-include '
        'that commit verbatim, or leave it empty to have the LLM re-analyze it on the next '
        '`refine` run.\n'
    )

    excluded_commits = [commit for commit in commits if commit.sha not in failed]
    # Largest commits first: the biggest changes are the most important to review.
    excluded_commits.sort(key=lambda commit: commit.score_size, reverse=True)

    lines.append(f'## Excluded ({len(excluded_commits)})\n')
    for commit in excluded_commits:
        lines.extend(_format_excluded_item(commit, github_url))
        lines.append('')

    if failed:
        lines.append(f'{_UNPROCESSED_SECTION_TITLE} ({len(failed)})\n')
        failed_commits = [commit for commit in commits if commit.sha in failed]
        for commit in failed_commits:
            lines.extend(_format_failed_item(commit, failed[commit.sha], github_url))
            lines.append('')

    return '\n'.join(lines)


def _format_excluded_item(commit: ClassifiedCommit, github_url: str) -> list[str]:
    short_sha = commit.sha[:8]
    commit_url = f'{github_url}/commit/{commit.sha}'
    changelog_line = commit.changelog_line or ''

    return [
        f'- [ ] [`{short_sha}`]({commit_url}) {commit.title}',
        f'      classification: {commit.classification.value}',
        f'      changelog_line: {changelog_line}',
        f'      analysis: {commit.commit_analysis or ""}',
    ]


def _format_failed_item(commit: ClassifiedCommit, error: str, github_url: str) -> list[str]:
    short_sha = commit.sha[:8]
    commit_url = f'{github_url}/commit/{commit.sha}'

    return [
        f'- [ ] [`{short_sha}`]({commit_url}) {commit.title}',
        '      classification: unclear',
        '      changelog_line: ',
        f'      error: {error}',
    ]
