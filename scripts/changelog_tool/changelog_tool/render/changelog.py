"""Render the final changelog.md from a list of classified commits."""

from collections.abc import Callable
import re

from changelog_tool.common.models import ClassifiedCommit
from changelog_tool.common.models import ExternalGroup

# Defense in depth: only these three classifications may appear in the main
# sections, even if to_changelog is somehow true for others.
_CLASSIFICATION_ORDER: tuple[str, ...] = (
    'breaking-change',
    'feature',
    'optimization',
)

_SECTION_TITLES: dict[str, str] = {
    'breaking-change': 'Breaking Change',
    'feature': 'Features',
    'optimization': 'Optimizations',
}

_EXTERNAL_GROUP_ORDER: tuple[str, ...] = (
    ExternalGroup.BUILD.value,
    ExternalGroup.DOCUMENTATION.value,
    ExternalGroup.FIXES.value,
    ExternalGroup.OTHER.value,
)

_NEEDS_ATTENTION_MARKER = '\u26a0 analyzed without diff \u2014 verify'

_FORCED_SECTION_TITLE = 'Forced'


def render_changelog(
    commits: list[ClassifiedCommit],
    github_url: str,
) -> str:
    lines: list[str] = []

    groups: dict[str, list[ClassifiedCommit]] = {}
    forced_commits: list[ClassifiedCommit] = []
    for commit in commits:
        if commit.to_changelog is True and commit.changelog_line:
            classification = commit.classification.value
            if classification in _CLASSIFICATION_ORDER:
                groups.setdefault(classification, []).append(commit)
            else:
                # Human-forced commit with a non-top-3 classification — render
                # in the Forced section instead of silently dropping it.
                forced_commits.append(commit)

    for classification in _CLASSIFICATION_ORDER:
        if classification not in groups:
            continue

        section_commits = groups[classification]
        if not section_commits:
            continue

        section_title = _SECTION_TITLES[classification]
        lines.append(f'{section_title}:')
        lines.append('')

        lines.extend(_render_grouped_by_component(section_commits, _format_changelog_entry))

    if forced_commits:
        lines.extend(_render_forced_section(forced_commits))

    external_leftovers = [
        commit
        for commit in commits
        if commit.is_external and not (commit.to_changelog is True and commit.changelog_line)
    ]

    lines.extend(_render_external_sections(external_leftovers))

    return '\n'.join(lines)


def _render_external_sections(external_leftovers: list[ClassifiedCommit]) -> list[str]:
    lines: list[str] = []

    external_groups: dict[str, list[ClassifiedCommit]] = {}
    for commit in external_leftovers:
        # Leftovers without a group fall back to "Other" instead of being dropped.
        group = commit.external_group.value if commit.external_group else ExternalGroup.OTHER.value
        external_groups.setdefault(group, []).append(commit)

    for group in _EXTERNAL_GROUP_ORDER:
        group_commits = external_groups.get(group)
        if not group_commits:
            continue

        lines.append(f'{group}:')
        lines.append('')

        if group == ExternalGroup.FIXES.value:
            lines.extend(_render_grouped_by_component(group_commits, _format_external_entry))
        else:
            for commit in group_commits:
                lines.extend(_format_external_entry(commit, indent=2))
            lines.append('')

    return lines


def _render_forced_section(forced_commits: list[ClassifiedCommit]) -> list[str]:
    lines: list[str] = [f'{_FORCED_SECTION_TITLE}:', '']
    lines.extend(_render_grouped_by_component(forced_commits, _format_changelog_entry))
    return lines


def _render_grouped_by_component(
    commits: list[ClassifiedCommit],
    format_fn: Callable[[ClassifiedCommit, int], list[str]],
) -> list[str]:
    lines: list[str] = []

    component_groups: dict[str, list[ClassifiedCommit]] = {}
    commits_without_component: list[ClassifiedCommit] = []

    for commit in commits:
        if commit.component:
            component_groups.setdefault(commit.component, []).append(commit)
        else:
            commits_without_component.append(commit)

    for component in sorted(component_groups.keys()):
        lines.append(f'  * {component}')
        lines.append('')
        for commit in component_groups[component]:
            lines.extend(format_fn(commit, indent=4))
        lines.append('')

    if commits_without_component:
        for commit in commits_without_component:
            lines.extend(format_fn(commit, indent=2))
        lines.append('')

    return lines


def _format_changelog_entry(commit: ClassifiedCommit, indent: int) -> list[str]:
    short_sha = commit.sha[:8]
    prefix = ' ' * indent
    line = f'{prefix}* {commit.changelog_line} <!-- {short_sha} -->'

    if commit.is_external:
        author_link = _format_author_link(commit)
        line += f' Many thanks to {author_link} for the PR!'

    if commit.needs_attention:
        line += f' {_NEEDS_ATTENTION_MARKER}'

    result = [line]

    if commit.classification.value == 'breaking-change' and commit.migration_guide:
        migration_prefix = ' ' * (indent + 2)
        result.append(f'{migration_prefix}* Migration: {commit.migration_guide}')

    return result


def _format_external_entry(commit: ClassifiedCommit, indent: int) -> list[str]:
    short_sha = commit.sha[:8]
    prefix = ' ' * indent
    author_link = _format_author_link(commit)
    line = f'{prefix}* {commit.changelog_line} Many thanks to {author_link} for the PR! <!-- {short_sha} -->'

    if commit.needs_attention:
        line += f' {_NEEDS_ATTENTION_MARKER}'

    return [line]


def _format_author_link(commit: ClassifiedCommit) -> str:
    """Format the contributor name as a Markdown link to their GitHub profile.

    When GitHub profile data is available, produces ``[Name](profile_url)``.
    Falls back to the plain-text git author name when no profile was resolved.
    """
    if commit.github_profile_url:
        display_name = commit.github_name or commit.github_login
        return f'[{display_name}]({commit.github_profile_url})'
    return _extract_author_name(commit.author)


def _extract_author_name(author: str) -> str:
    match = re.match(r'^(.+?)\s*<', author)
    if match:
        return match.group(1).strip()
    return author
