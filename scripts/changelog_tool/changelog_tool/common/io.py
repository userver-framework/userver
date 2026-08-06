"""Atomic file I/O for classified commit data."""

import json
import logging
import os
import pathlib
import re
import tempfile

from changelog_tool.common.models import ClassifiedCommit

logger = logging.getLogger(__name__)

# Captures the full 40-char SHA from the commit URL to avoid short-SHA collisions.
_CHECKED_ITEM_RE = re.compile(r'^-\s*\[[xX]\]\s*\[`[0-9a-f]+`\]\(.*?/commit/([0-9a-f]{40})\)')

_ANY_ITEM_RE = re.compile(r'^-\s*\[[ xX]\]\s*')

_CHANGELOG_LINE_RE = re.compile(r'^\s*changelog_line:\s*(.*)$')


def dump_classified_commits(
    commits: list[ClassifiedCommit],
    output_dir: pathlib.Path,
    filename: str,
) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    output_file = output_dir / filename

    json_data = [commit.model_dump() for commit in commits]
    json_str = json.dumps(json_data, indent=2, ensure_ascii=False)

    atomic_write_text(output_file, json_str)
    logger.info('Wrote %d commits to %s', len(commits), output_file)


def load_classified_commits(
    output_dir: pathlib.Path,
    filename: str,
) -> list[ClassifiedCommit]:
    input_file = output_dir / filename

    if not input_file.exists():
        return []

    with open(input_file, encoding='utf-8') as f:
        json_data = json.load(f)

    return [ClassifiedCommit(**item) for item in json_data]


def parse_excluded_actions(path: pathlib.Path) -> dict[str, str | None]:
    """Parse `excluded.md` and return the reviewer's requested actions.

    For each checked item, returns `{full_sha: changelog_line}` where
    `changelog_line` is None if empty (re-analyze) or the text (force-include).
    """
    if not path.exists():
        return {}

    with open(path, encoding='utf-8') as f:
        lines = f.readlines()

    actions: dict[str, str | None] = {}
    current_sha: str | None = None

    for line in lines:
        stripped_line = line.rstrip('\n')

        item_match = _CHECKED_ITEM_RE.match(stripped_line)
        if item_match:
            current_sha = item_match.group(1)
            actions[current_sha] = None
            continue

        if _ANY_ITEM_RE.match(stripped_line):
            current_sha = None
            continue

        if current_sha is None:
            continue

        changelog_match = _CHANGELOG_LINE_RE.match(stripped_line)
        if changelog_match:
            value = changelog_match.group(1).strip()
            actions[current_sha] = value or None

    return actions


def atomic_write_text(path: pathlib.Path, content: str) -> None:
    """Write text to a file atomically using a temporary file."""
    dir_path = path.parent
    dir_path.mkdir(parents=True, exist_ok=True)

    fd, tmp_path = tempfile.mkstemp(dir=dir_path, prefix='.tmp_', suffix=path.suffix)
    try:
        with os.fdopen(fd, 'w', encoding='utf-8') as f:
            f.write(content)
        os.replace(tmp_path, path)
    except BaseException:
        try:
            os.unlink(tmp_path)
        except OSError:
            pass
        raise
