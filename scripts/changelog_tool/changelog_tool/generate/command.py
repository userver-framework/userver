"""Generate command: fetch commits, classify via LLM, render changelog + excluded."""

from __future__ import annotations

import asyncio
import logging
import re

from changelog_tool.app import AppContainer
import changelog_tool.common.git as git
from changelog_tool.common.github import GitHubProfileResolver
import changelog_tool.common.io as io
from changelog_tool.common.models import ClassifiedCommit
from changelog_tool.config import Config
from changelog_tool.pipeline import EXIT_HARD_FAILURE
from changelog_tool.pipeline import run_llm_passes

logger = logging.getLogger(__name__)


def _extract_component_from_title(title: str) -> str | None:
    match = re.match(r'^(\w+)(?:\(([^)]+)\))?:?\s*(.+)', title)
    if not match:
        return None

    component = match.group(2)
    description = match.group(3)

    if component:
        return component.lower()

    words = description.split()
    if not words:
        return None

    if words[0].endswith(':'):
        return words[0][:-1].lower()

    if len(words) > 1 and words[1].startswith(':'):
        return words[0].lower()

    return None


def generate(config: Config) -> int:
    logger.info('Generating changelog for commits from %s to %s...', config.generate.from_sha, config.generate.to_sha)

    try:
        commits: list[git.Commit] = git.get_commits(
            config.generate.from_sha, config.generate.to_sha, config.generate.repo_path
        )
    except git.GitError as e:
        logger.error('Failed to fetch commits: %s', e)
        return EXIT_HARD_FAILURE

    core_team_regexes = [re.compile(pattern) for pattern in config.generate.core_team_patterns]

    classified_commits: list[ClassifiedCommit] = []
    for commit in commits:
        is_core_team = any(regex.match(commit.author) for regex in core_team_regexes)
        component = _extract_component_from_title(commit.title)

        classified_commits.append(
            ClassifiedCommit(
                **commit.model_dump(),
                is_external=not is_core_team,
                component=component,
            )
        )

    logger.info('Found %d commits', len(classified_commits))

    container = AppContainer(config)

    # All async work must run inside a single asyncio.run() call: httpx binds
    # client connections to the event loop that was active when they were
    # opened. Calling asyncio.run() again for client.close() would run on a
    # different loop and fail with "RuntimeError: Event loop is closed".
    rounds, failed_shas = asyncio.run(_run_async_pipeline(container, classified_commits, config))

    io.dump_classified_commits(classified_commits, config.generate.output_dir, 'classified.json')

    return container.output_renderer.render_and_summarize(classified_commits, failed_shas, rounds)


async def _run_async_pipeline(
    container: AppContainer,
    classified_commits: list[ClassifiedCommit],
    config: Config,
) -> tuple[int, dict[str, str]]:
    """Run LLM passes and GitHub profile resolution in a single event loop."""
    rounds, failed_shas = await run_llm_passes(container.processor, classified_commits, config, container.resume_loop)

    external_commits = [c for c in classified_commits if c.is_external]
    if external_commits:
        resolver = GitHubProfileResolver(
            github_url=config.render.github_url,
            token=config.render.github_token,
        )
        await resolver.resolve_profiles(external_commits)

    return rounds, failed_shas
