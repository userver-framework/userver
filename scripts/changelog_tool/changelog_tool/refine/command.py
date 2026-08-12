"""Refine command: act on reviewer decisions recorded in excluded.md."""

from __future__ import annotations

import asyncio
import logging

from changelog_tool.app import AppContainer
import changelog_tool.common.io as io
from changelog_tool.common.models import ClassifiedCommit
from changelog_tool.config import Config
from changelog_tool.pipeline import EXIT_HARD_FAILURE
from changelog_tool.pipeline import run_refine_passes

logger = logging.getLogger(__name__)


def refine(config: Config) -> int:
    classified_commits: list[ClassifiedCommit] = io.load_classified_commits(
        config.generate.output_dir, 'classified.json'
    )

    if not classified_commits:
        logger.warning("No classified commits found. Please run 'generate' first.")
        return EXIT_HARD_FAILURE

    excluded_path = config.render.output_dir / 'excluded.md'
    actions = io.parse_excluded_actions(excluded_path)

    if not actions:
        logger.info('No checked items found in %s; nothing to refine.', excluded_path)
    else:
        logger.info('Found %d checked item(s) in %s', len(actions), excluded_path)

    commit_map = {commit.sha: commit for commit in classified_commits}

    container = AppContainer(config)

    to_reanalyze: list[ClassifiedCommit] = []
    force_included: list[tuple[str, str]] = []

    for sha, changelog_line in actions.items():
        commit = commit_map.get(sha)
        if commit is None:
            logger.warning('excluded.md references unknown commit %s; skipping', sha)
            continue

        if changelog_line:
            # Force-include verbatim, no LLM call. Clear external_group so the
            # commit doesn't linger in both a main section and a bottom section.
            commit.to_changelog = True
            commit.changelog_line = changelog_line
            commit.external_group = None
            force_included.append((sha, changelog_line))
        else:
            to_reanalyze.append(commit)

    for sha, _ in force_included:
        logger.info('Force-including commit %s verbatim', sha[:8])

    # See generate/command.py for why all async work must be in a single
    # asyncio.run() call.
    rounds, failed_shas = asyncio.run(
        run_refine_passes(container.processor, to_reanalyze, config, container.resume_loop)
    )

    io.dump_classified_commits(classified_commits, config.generate.output_dir, 'classified.json')

    return container.output_renderer.render_and_summarize(classified_commits, failed_shas, rounds)
