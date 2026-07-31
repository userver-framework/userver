"""Git operations: fetching commits, diffs, and parsing git output."""

from __future__ import annotations

import logging
from pathlib import Path
import re
import subprocess

import pydantic

logger = logging.getLogger(__name__)

GIT_TIMEOUT: int = 120


class GitError(Exception):
    """Any error when working with git."""


class FileChange(pydantic.BaseModel):
    path: str
    old_path: str | None = None
    added_lines: int = 0
    removed_lines: int = 0


class Commit(pydantic.BaseModel):
    sha: str
    title: str
    message: str
    author: str
    co_authors: list[str]
    changed_files: list[FileChange]
    total_added: int = 0
    total_removed: int = 0
    score_size: int = 0


def get_commits(
    from_ref: str | None = None,
    to_ref: str = 'HEAD',
    repo_path: str | Path | None = None,
) -> list[Commit]:
    cwd = repo_path
    rev_range = f'{from_ref}..{to_ref}' if from_ref else to_ref

    raw_shas = _run_git(['log', '--format=%H', rev_range], cwd)
    shas = [s.strip() for s in raw_shas.splitlines() if s.strip()]

    logger.info('Found %d commits in range %s', len(shas), rev_range)

    return [_fetch_commit(sha, cwd) for sha in shas]


def get_commit_diff(commit: Commit, repo_path: str | Path | None = None) -> str:
    return get_diff_by_sha(commit.sha, repo_path)


def get_diff_by_sha(sha: str, repo_path: str | Path | None = None) -> str:
    return _run_git(['diff-tree', '--root', '-p', '-r', '-M', sha], repo_path)


def _run_git(args: list[str], cwd: Path) -> str:
    try:
        result = subprocess.run(
            ['git', *args],
            cwd=cwd,
            capture_output=True,
            text=True,
            timeout=GIT_TIMEOUT,
        )
    except FileNotFoundError:
        raise GitError('git executable not found')
    except subprocess.TimeoutExpired:
        raise GitError(f'git {" ".join(args)} timed out after {GIT_TIMEOUT}s')

    if result.returncode != 0:
        raise GitError(result.stderr.strip() or f'git {args[0]} failed')

    return result.stdout


def _parse_rename(path_str: str) -> tuple[str, str | None]:
    m = re.match(r'^(.*?)\{(.*?) => (.*?)\}(.*)$', path_str)
    if m:
        pre, old_mid, new_mid, suf = m.groups()
        old = (pre + old_mid + suf).strip('/')
        new = (pre + new_mid + suf).strip('/')
        return new, old

    if ' => ' in path_str:
        old, new = path_str.split(' => ', 1)
        return new.strip(), old.strip()

    return path_str, None


def _parse_numstat(output: str) -> list[FileChange]:
    changes: list[FileChange] = []

    for line in output.splitlines():
        line = line.strip()
        if not line:
            continue

        parts = line.split('\t', 2)
        if len(parts) != 3:
            continue

        added_str, removed_str, path_str = parts

        added = 0 if added_str == '-' else int(added_str)
        removed = 0 if removed_str == '-' else int(removed_str)

        path, old_path = _parse_rename(path_str)
        changes.append(
            FileChange(
                path=path,
                old_path=old_path,
                added_lines=added,
                removed_lines=removed,
            )
        )

    return changes


def _parse_co_authors(message: str) -> list[str]:
    return re.findall(r'(?im)^Co-authored-by:\s*(.+)$', message)


def _fetch_commit(sha: str, cwd: Path) -> Commit:
    raw_meta = _run_git(
        ['show', '-s', '--format=%H%x00%an <%ae>%x00%B', sha],
        cwd,
    )
    parts = raw_meta.split('\x00', 2)
    if len(parts) < 3:
        raise GitError(f'Unexpected git show output for {sha!r}')

    sha_full = parts[0].strip()
    author = parts[1].strip()
    message = parts[2].strip()
    title = message.splitlines()[0] if message else ''

    raw_numstat = _run_git(
        ['diff-tree', '--root', '--numstat', '-r', '-M', sha],
        cwd,
    )
    changes = _parse_numstat(raw_numstat)

    return Commit(
        sha=sha_full,
        title=title,
        message=message,
        author=author,
        co_authors=_parse_co_authors(message),
        changed_files=changes,
        total_added=sum(c.added_lines for c in changes),
        total_removed=sum(c.removed_lines for c in changes),
        score_size=sum(c.added_lines + c.removed_lines for c in changes),
    )
