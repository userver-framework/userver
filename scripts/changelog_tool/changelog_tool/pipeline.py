"""Shared pipeline orchestration: resume loop, result application, output rendering."""

from __future__ import annotations

import asyncio
from collections.abc import Awaitable
from collections.abc import Callable
import logging

import changelog_tool.common.io as io
from changelog_tool.common.models import Classification
from changelog_tool.common.models import ClassifiedCommit
from changelog_tool.common.models import ExternalGroup
from changelog_tool.config import Config
from changelog_tool.llm.processor import LLMProcessor
from changelog_tool.llm.state import LLMState
from changelog_tool.render.changelog import render_changelog
from changelog_tool.render.excluded import render_excluded

logger = logging.getLogger(__name__)

EXIT_OK = 0
EXIT_PARTIAL = 2
EXIT_HARD_FAILURE = 1


class ResumeLoop:
    """Drive an LLM processor pass to completion across retry rounds."""

    def __init__(self, config: Config) -> None:
        self._max_rounds = config.llm_config.max_rounds
        self._round_delay_seconds = config.llm_config.round_delay_seconds

    async def run(
        self,
        commits: list[ClassifiedCommit],
        *,
        process_fn: Callable[[list[ClassifiedCommit]], Awaitable[dict]],
        state: LLMState,
        log_prefix: str = '',
    ) -> tuple[dict, int]:
        results: dict = {}
        all_shas = {c.sha for c in commits}
        previous_pending_count: int | None = None
        rounds = 0

        for round_num in range(1, self._max_rounds + 1):
            rounds = round_num
            round_results = await process_fn(commits)
            results.update(round_results)

            pending = await state.pending_shas(all_shas)
            if not pending:
                break

            if previous_pending_count is not None and len(pending) >= previous_pending_count:
                logger.warning(
                    '%sRound %d made no progress (%d commits still pending); stopping resume loop',
                    log_prefix,
                    round_num,
                    len(pending),
                )
                break

            previous_pending_count = len(pending)

            if round_num < self._max_rounds:
                logger.info(
                    '%sRound %d: %d commits still pending, sleeping %ds before retry...',
                    log_prefix,
                    round_num,
                    len(pending),
                    self._round_delay_seconds,
                )
                await asyncio.sleep(self._round_delay_seconds)

        return results, rounds


class ResultApplicator:
    """Apply raw LLM result dicts to `ClassifiedCommit` objects."""

    @staticmethod
    def apply_main_results(commits: list[ClassifiedCommit], llm_results: dict) -> None:
        for commit in commits:
            result = llm_results.get(commit.sha)
            if not result:
                continue

            try:
                commit.classification = Classification(result.get('classification', 'unclear'))
            except ValueError:
                logger.warning('Unknown classification for commit %s, keeping UNCLEAR', commit.sha)

            commit.to_changelog = result.get('to_changelog')
            commit.changelog_line = result.get('changelog_line')
            commit.commit_analysis = result.get('detailed_commit_analysis')
            if result.get('component'):
                commit.component = result.get('component')
            commit.needs_attention = bool(result.get('needs_attention', False))
            commit.migration_guide = result.get('migration_guide')

    @staticmethod
    def apply_external_results(commits: list[ClassifiedCommit], external_results: dict) -> None:
        valid_groups = {group.value for group in ExternalGroup}

        for commit in commits:
            result = external_results.get(commit.sha)
            if not result:
                continue

            changelog_line = result.get('changelog_line') or None
            promote_to_feature = bool(result.get('promote_to_feature', False))

            if promote_to_feature and changelog_line:
                commit.classification = Classification.FEATURE
                commit.to_changelog = True
                commit.changelog_line = changelog_line
                continue

            group = result.get('group')
            commit.external_group = ExternalGroup(group) if group in valid_groups else None
            commit.changelog_line = changelog_line
            if commit.external_group == ExternalGroup.FIXES and result.get('component'):
                commit.component = result.get('component')


class OutputRenderer:
    """Render changelog.md + excluded.md, write them, print the summary."""

    def __init__(self, config: Config) -> None:
        self._config = config

    def render_and_summarize(
        self,
        classified_commits: list[ClassifiedCommit],
        failed_shas: dict[str, str],
        rounds: int,
    ) -> int:
        changelog_content = render_changelog(
            classified_commits,
            self._config.render.github_url,
        )
        excluded_commits = [c for c in classified_commits if not (c.to_changelog is True and c.changelog_line)]
        excluded_content = render_excluded(excluded_commits, failed_shas, self._config.render.github_url)

        changelog_path = self._config.render.output_dir / 'changelog.md'
        excluded_path = self._config.render.output_dir / 'excluded.md'
        io.atomic_write_text(changelog_path, changelog_content)
        io.atomic_write_text(excluded_path, excluded_content)
        logger.info('Wrote changelog to %s', changelog_path)
        logger.info('Wrote excluded list to %s', excluded_path)
        print(f'changelog: {changelog_path}')
        print(f'excluded: {excluded_path}')

        processed = len(classified_commits) - len(failed_shas)
        needs_attention = sum(1 for c in classified_commits if c.needs_attention)
        logger.info(
            'Done: processed=%d failed=%d needs_attention=%d rounds=%d',
            processed,
            len(failed_shas),
            needs_attention,
            rounds,
        )
        print(f'processed={processed} failed={len(failed_shas)} needs_attention={needs_attention} rounds={rounds}')

        return EXIT_PARTIAL if failed_shas else EXIT_OK


async def run_llm_passes(
    processor: LLMProcessor,
    classified_commits: list[ClassifiedCommit],
    config: Config,
    resume_loop: ResumeLoop,
) -> tuple[int, dict[str, str]]:
    try:
        llm_results, rounds = await resume_loop.run(
            classified_commits,
            process_fn=processor.process_commits,
            state=processor.state,
        )
        ResultApplicator.apply_main_results(classified_commits, llm_results)

        external_leftovers = [
            c for c in classified_commits if c.is_external and not (c.to_changelog is True and c.changelog_line)
        ]
        if external_leftovers:
            external_results, ext_rounds = await resume_loop.run(
                external_leftovers,
                process_fn=processor.process_external_commits,
                state=processor.external_state,
                log_prefix='External ',
            )
            ResultApplicator.apply_external_results(external_leftovers, external_results)
            rounds = max(rounds, ext_rounds)

        failed_shas = await processor.state.failed_shas()
        failed_shas = {**failed_shas, **await processor.external_state.failed_shas()}

        return rounds, failed_shas
    finally:
        await processor.llm_client.close()


async def run_refine_passes(
    processor: LLMProcessor,
    to_reanalyze: list[ClassifiedCommit],
    config: Config,
    resume_loop: ResumeLoop,
) -> tuple[int, dict[str, str]]:
    try:
        # Must load state before drop_result(): otherwise the in-memory state
        # is empty and drop_result() is a no-op, so the stale on-disk entry
        # would still satisfy get_result() inside process_commits() and the
        # commit would never actually be re-sent to the LLM.
        await processor.state.load()
        for commit in to_reanalyze:
            await processor.state.drop_result(commit.sha)

        if to_reanalyze:
            logger.info('Re-analyzing %d commit(s) via LLM...', len(to_reanalyze))
            llm_results, rounds = await resume_loop.run(
                to_reanalyze,
                process_fn=processor.process_commits,
                state=processor.state,
            )
            ResultApplicator.apply_main_results(to_reanalyze, llm_results)
        else:
            rounds = 0

        failed_shas = await processor.state.failed_shas()
        failed_shas = {**failed_shas, **await processor.external_state.failed_shas()}

        return rounds, failed_shas
    finally:
        await processor.llm_client.close()
