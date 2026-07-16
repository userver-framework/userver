"""Batch processing of commits through the LLM for classification."""

import asyncio
import json
import logging
from pathlib import Path
from typing import Any

from changelog_tool.common.git import Commit
from changelog_tool.common.git import get_commit_diff
from changelog_tool.llm.client import BaseLLMClient
from changelog_tool.llm.config import LLMConfig
from changelog_tool.llm.exceptions import LLMError
from changelog_tool.llm.prompts import EXTERNAL_CONTRIB_SYSTEM_PROMPT
from changelog_tool.llm.prompts import SYSTEM_PROMPT
from changelog_tool.llm.state import LLMState

logger = logging.getLogger(__name__)


class LLMProcessor:
    """Processes commits through the LLM in batches."""

    def __init__(self, config: LLMConfig, llm_client: BaseLLMClient, output_dir: Path, repo_path: Path) -> None:
        self.config = config
        self.llm_client = llm_client
        self.output_dir = output_dir
        self.state = LLMState(output_dir / 'llm_state.json')
        # Separate state file so a SHA processed by both passes never has one
        # result clobber the other.
        self.external_state = LLMState(output_dir / 'external_state.json')
        self.repo_path = repo_path

    async def process_commits(self, commits: list[Commit]) -> dict[str, dict[str, Any]]:
        await self.state.load()
        valid_shas = {commit.sha for commit in commits}
        await self.state.cleanup(valid_shas)

        commits_to_process: list[Commit] = []
        results: dict[str, dict[str, Any]] = {}

        for commit in commits:
            result = await self.state.get_result(commit.sha)
            if result:
                results[commit.sha] = result
            else:
                commits_to_process.append(commit)

        logger.info(
            'Found %d commits, %d already processed, %d to process via LLM',
            len(commits),
            len(results),
            len(commits_to_process),
        )

        if not commits_to_process:
            return results

        batches, diffless_shas = self._create_smart_batches(commits_to_process)

        total_commits = sum(len(batch) for batch in batches)
        logger.info('Processing %d commits in %d batches...', total_commits, len(batches))

        batch_results = await asyncio.gather(
            *[
                self._process_batch(batch, diffless_shas, i, len(batches), total_commits)
                for i, batch in enumerate(batches)
            ],
            return_exceptions=True,
        )

        completed_batches = 0
        for batch_idx, batch_result in enumerate(batch_results):
            if isinstance(batch_result, Exception):
                logger.error('Batch %d/%d failed: %s', batch_idx + 1, len(batches), batch_result)
                continue
            completed_batches += 1
            results.update(batch_result)

        logger.info('Completed %d/%d batches', completed_batches, len(batches))

        return results

    async def process_external_commits(self, commits: list[Commit]) -> dict[str, dict[str, Any]]:
        """Creative external-contribution pass (Part B). Diffless by design."""
        await self.external_state.load()
        valid_shas = {commit.sha for commit in commits}
        await self.external_state.cleanup(valid_shas)

        commits_to_process: list[Commit] = []
        results: dict[str, dict[str, Any]] = {}

        for commit in commits:
            result = await self.external_state.get_result(commit.sha)
            if result:
                results[commit.sha] = result
            else:
                commits_to_process.append(commit)

        logger.info(
            'External pass: found %d commits, %d already processed, %d to process via LLM',
            len(commits),
            len(results),
            len(commits_to_process),
        )

        if not commits_to_process:
            return results

        batches = self._create_external_batches(commits_to_process)

        total_commits = sum(len(batch) for batch in batches)
        logger.info('External pass: processing %d commits in %d batches...', total_commits, len(batches))

        batch_results = await asyncio.gather(
            *[self._process_external_batch(batch, i, len(batches)) for i, batch in enumerate(batches)],
            return_exceptions=True,
        )

        completed_batches = 0
        for batch_idx, batch_result in enumerate(batch_results):
            if isinstance(batch_result, Exception):
                logger.error('External batch %d/%d failed: %s', batch_idx + 1, len(batches), batch_result)
                continue
            completed_batches += 1
            results.update(batch_result)

        logger.info('External pass: completed %d/%d batches', completed_batches, len(batches))

        return results

    async def _process_external_batch(
        self,
        batch: list[Commit],
        batch_idx: int = 0,
        total_batches: int = 0,
    ) -> dict[str, dict[str, Any]]:
        try:
            logger.info('External [%d/%d] Processing %d commits...', batch_idx + 1, total_batches, len(batch))
            system_prompt, user_prompt = self._build_external_prompt(batch)

            response_text = await self.llm_client.generate(system_prompt, user_prompt)
            response_text = _strip_code_blocks(response_text)

            try:
                response_data = json.loads(response_text)
            except json.JSONDecodeError as e:
                raise LLMError(f'LLM returned invalid JSON: {e}') from e

            if not isinstance(response_data, dict):
                raise LLMError('LLM returned invalid response format (not a dict)')

            results: dict[str, dict[str, Any]] = {}
            for commit in batch:
                commit_data = response_data.get(commit.sha, {})
                if not isinstance(commit_data, dict):
                    commit_data = {}

                group = commit_data.get('group')
                changelog_line = commit_data.get('changelog_line', '')
                component = commit_data.get('component')
                promote_to_feature = bool(commit_data.get('promote_to_feature', False))

                data = {
                    'group': group,
                    'changelog_line': changelog_line,
                    'component': component,
                    'promote_to_feature': promote_to_feature,
                }

                await self.external_state.set_raw_result(commit.sha, data)
                results[commit.sha] = data

            logger.info('External [%d/%d] Completed', batch_idx + 1, total_batches)
            return results

        except Exception as e:
            # Mark errored commits so they surface in failed_shas() and are
            # retried by the resume loop instead of silently vanishing.
            error_msg = f'{type(e).__name__}: {e}'
            logger.error('External batch %d/%d failed: %s', batch_idx + 1, total_batches, error_msg)
            for commit in batch:
                await self.external_state.set_error(commit.sha, error_msg)
            return {
                commit.sha: {
                    'group': None,
                    'changelog_line': '',
                    'component': None,
                    'promote_to_feature': False,
                }
                for commit in batch
            }

    def _create_external_batches(self, commits: list[Commit]) -> list[list[Commit]]:
        if not commits:
            return []

        batches: list[list[Commit]] = []
        current_batch: list[Commit] = []
        current_prompt_size = 0

        system_prompt_size = sum(len(part) for part in self._build_external_prompt([]))

        for commit in commits:
            commit_prompt_size = self._estimate_commit_size(commit, include_diff=False)

            can_add = (
                len(current_batch) < self.config.max_commits_per_batch
                and (current_prompt_size + commit_prompt_size + system_prompt_size)
                <= self.config.max_user_prompt_length
            )

            if can_add:
                current_batch.append(commit)
                current_prompt_size += commit_prompt_size
            else:
                if current_batch:
                    batches.append(current_batch)
                current_batch = [commit]
                current_prompt_size = commit_prompt_size

        if current_batch:
            batches.append(current_batch)

        return batches

    def _build_external_prompt(self, commits: list[Commit]) -> tuple[str, str]:
        user_parts = [self._format_commit_metadata(commit) for commit in commits]
        user_prompt = 'Please analyze the following external commits:\n\n' + '\n---\n'.join(user_parts)
        return EXTERNAL_CONTRIB_SYSTEM_PROMPT, user_prompt

    async def _process_batch(
        self,
        batch: list[Commit],
        diffless_shas: set[str],
        batch_idx: int = 0,
        total_batches: int = 0,
        total_commits: int = 0,
    ) -> dict[str, dict[str, Any]]:
        try:
            logger.info('[%d/%d] Processing %d commits...', batch_idx + 1, total_batches, len(batch))
            system_prompt, user_prompt = self._build_prompt(batch, diffless_shas)

            response_text = await self.llm_client.generate(system_prompt, user_prompt)
            response_text = _strip_code_blocks(response_text)

            try:
                response_data = json.loads(response_text)
            except json.JSONDecodeError as e:
                raise LLMError(f'LLM returned invalid JSON: {e}') from e

            if not isinstance(response_data, dict):
                raise LLMError('LLM returned invalid response format (not a dict)')

            results: dict[str, dict[str, Any]] = {}
            for commit in batch:
                commit_data = response_data.get(commit.sha, {})
                if isinstance(commit_data, str):
                    classification = commit_data
                    to_changelog = classification in ('feature', 'breaking-change')
                    changelog_line = ''
                    detailed_commit_analysis = ''
                    component = None
                    migration_guide = None
                else:
                    classification = commit_data.get('classification', 'unclear')
                    to_changelog = commit_data.get('to_changelog')
                    changelog_line = commit_data.get('changelog_line', '')
                    detailed_commit_analysis = commit_data.get('detailed_commit_analysis', '')
                    component = commit_data.get('component')
                    migration_guide = commit_data.get('migration_guide')

                needs_attention = commit.sha in diffless_shas

                completed = await self.state.set_result(
                    commit.sha,
                    classification,
                    changelog_line,
                    detailed_commit_analysis,
                    to_changelog,
                    component=component,
                    needs_attention=needs_attention,
                    migration_guide=migration_guide,
                )
                if total_commits > 0:
                    remaining = total_commits - completed
                    logger.info('  Progress: %d/%d commits, %d remaining', completed, total_commits, remaining)

                results[commit.sha] = {
                    'classification': classification,
                    'to_changelog': to_changelog,
                    'changelog_line': changelog_line,
                    'detailed_commit_analysis': detailed_commit_analysis,
                    'component': component,
                    'needs_attention': needs_attention,
                    'migration_guide': migration_guide,
                }

            logger.info('[%d/%d] Completed', batch_idx + 1, total_batches)
            return results

        except Exception as e:
            # Mark errored commits so they surface in failed_shas() and are
            # retried by the resume loop instead of silently vanishing.
            error_msg = f'{type(e).__name__}: {e}'
            logger.error('Batch %d/%d failed: %s', batch_idx + 1, total_batches, error_msg)
            for commit in batch:
                await self.state.set_error(commit.sha, error_msg)
            return _build_error_results(batch)

    def _create_smart_batches(self, commits: list[Commit]) -> tuple[list[list[Commit]], set[str]]:
        """Group commits into batches that fit the prompt size budget.

        Oversize commits are isolated into single-commit batches and rebuilt
        without their diff (flagged needs_attention). No commit is dropped.
        """
        if not commits:
            return [], set()

        batches: list[list[Commit]] = []
        current_batch: list[Commit] = []
        current_prompt_size = 0
        diffless_shas: set[str] = set()

        system_prompt_size = sum(len(part) for part in self._build_prompt([], set()))

        for commit in commits:
            commit_prompt_size = self._estimate_commit_size(commit)

            if system_prompt_size + commit_prompt_size > self.config.max_user_prompt_length:
                if current_batch:
                    batches.append(current_batch)
                    current_batch = []
                    current_prompt_size = 0

                diffless_shas.add(commit.sha)

                diffless_size = self._estimate_commit_size(commit, include_diff=False)
                if system_prompt_size + diffless_size > self.config.max_user_prompt_length:
                    logger.warning(
                        'Commit %s still exceeds max prompt length even without diff; sending anyway',
                        commit.sha,
                    )

                batches.append([commit])
                continue

            can_add = (
                len(current_batch) < self.config.max_commits_per_batch
                and (current_prompt_size + commit_prompt_size + system_prompt_size)
                <= self.config.max_user_prompt_length
            )

            if can_add:
                current_batch.append(commit)
                current_prompt_size += commit_prompt_size
            else:
                if current_batch:
                    batches.append(current_batch)
                current_batch = [commit]
                current_prompt_size = commit_prompt_size

        if current_batch:
            batches.append(current_batch)

        return batches, diffless_shas

    def _estimate_commit_size(self, commit: Commit, include_diff: bool = True) -> int:
        size = len(commit.sha) + len(commit.title) + len(commit.message)
        size += len(', '.join(f.path for f in commit.changed_files))

        if include_diff:
            diff = get_commit_diff(commit, self.repo_path)
            size += len(diff)

        return size + 200

    def _build_prompt(self, commits: list[Commit], diffless_shas: set[str] | None = None) -> tuple[str, str]:
        diffless_shas = diffless_shas or set()

        user_parts: list[str] = []
        for commit in commits:
            part = self._format_commit_metadata(commit)

            if commit.sha in diffless_shas:
                part += 'Diff: [omitted -- commit too large; analyze from title/message/files only]\n'
            else:
                diff = get_commit_diff(commit, self.repo_path)
                part += f'Diff:\n{diff}\n'

            user_parts.append(part)

        user_prompt = 'Please analyze the following commits:\n\n' + '\n---\n'.join(user_parts)
        return SYSTEM_PROMPT, user_prompt

    @staticmethod
    def _format_commit_metadata(commit: Commit) -> str:
        part = f'Commit SHA: {commit.sha}\n'
        part += f'Title: {commit.title}\n'
        part += f'Message: {commit.message}\n'
        part += f'Changed Files: {", ".join(f.path for f in commit.changed_files)}\n'
        return part


def _strip_code_blocks(text: str) -> str:
    stripped = text.strip()
    if stripped.startswith('```json'):
        stripped = stripped[7:]
    elif stripped.startswith('```'):
        stripped = stripped[3:]
    if stripped.endswith('```'):
        stripped = stripped[:-3]
    return stripped.strip()


def _build_error_results(batch: list[Commit]) -> dict[str, dict[str, Any]]:
    return {
        commit.sha: {
            'classification': 'unclear',
            'to_changelog': None,
            'changelog_line': '',
            'detailed_commit_analysis': '',
            'component': None,
            'needs_attention': False,
            'migration_guide': None,
        }
        for commit in batch
    }
