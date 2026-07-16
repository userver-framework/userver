"""Persistent state management for LLM-processed commits."""

import asyncio
import json
import logging
import os
from pathlib import Path
import tempfile
from typing import Any

logger = logging.getLogger(__name__)


class LLMState:
    """Manages persistent LLM analysis state across runs."""

    def __init__(self, state_file_path: Path) -> None:
        self.state_file_path = state_file_path
        self.state: dict[str, dict[str, Any]] = {}
        self._lock = asyncio.Lock()

    async def load(self) -> None:
        async with self._lock:
            if not self.state_file_path.exists():
                self.state = {}
                return

            try:
                with open(self.state_file_path, encoding='utf-8') as f:
                    loaded_state = json.load(f)
                if isinstance(loaded_state, dict):
                    self.state = loaded_state
                else:
                    logger.warning('State file %s is not a dict; resetting', self.state_file_path)
                    self.state = {}
            except (json.JSONDecodeError, OSError) as e:
                logger.warning('Could not load state file %s: %s', self.state_file_path, e)
                self.state = {}

    async def save(self) -> None:
        self.state_file_path.parent.mkdir(parents=True, exist_ok=True)

        try:
            await asyncio.to_thread(self._write_state)
        except OSError as e:
            logger.error('Could not save state file %s: %s', self.state_file_path, e)

    def _write_state(self) -> None:
        dir_path = self.state_file_path.parent
        fd, tmp_path = tempfile.mkstemp(dir=dir_path, prefix='.tmp_', suffix='.json')
        try:
            with os.fdopen(fd, 'w', encoding='utf-8') as f:
                json.dump(self.state, f, ensure_ascii=False, indent=2)
            os.replace(tmp_path, self.state_file_path)
        except BaseException:
            try:
                os.unlink(tmp_path)
            except OSError:
                pass
            raise

    async def cleanup(self, valid_shas: set[str]) -> None:
        async with self._lock:
            keys_to_remove = set(self.state.keys()) - valid_shas
            for key in keys_to_remove:
                del self.state[key]
            if keys_to_remove:
                logger.info('Cleaning up %d stale state entries', len(keys_to_remove))
                await self.save()

    async def get_result(self, sha: str) -> dict[str, Any] | None:
        async with self._lock:
            commit_data = self.state.get(sha)
            if commit_data and commit_data.get('error') is None:
                return commit_data
            return None

    async def set_result(
        self,
        sha: str,
        classification: str,
        changelog_line: str,
        detailed_commit_analysis: str,
        to_changelog: bool = False,
        component: str | None = None,
        needs_attention: bool = False,
        migration_guide: str | None = None,
    ) -> int:
        async with self._lock:
            self.state[sha] = {
                'classification': classification,
                'to_changelog': to_changelog,
                'changelog_line': changelog_line,
                'detailed_commit_analysis': detailed_commit_analysis,
                'component': component,
                'needs_attention': needs_attention,
                'migration_guide': migration_guide,
                'error': None,
            }
            completed = sum(1 for v in self.state.values() if v.get('error') is None)
            await self.save()
            return completed

    async def set_raw_result(self, sha: str, data: dict[str, Any]) -> int:
        async with self._lock:
            self.state[sha] = {**data, 'error': None}
            completed = sum(1 for v in self.state.values() if v.get('error') is None)
            await self.save()
            return completed

    async def set_error(self, sha: str, error_message: str) -> None:
        async with self._lock:
            self.state[sha] = {'classification': 'unclear', 'error': error_message}
            await self.save()

    async def drop_result(self, sha: str) -> None:
        async with self._lock:
            if sha in self.state:
                del self.state[sha]
                await self.save()

    async def pending_shas(self, all_shas: set[str]) -> set[str]:
        async with self._lock:
            return {sha for sha in all_shas if self.state.get(sha) is None or self.state[sha].get('error') is not None}

    async def failed_shas(self) -> dict[str, str]:
        async with self._lock:
            return {sha: data['error'] for sha, data in self.state.items() if data.get('error') is not None}
