"""Resolve GitHub profile information for external contributors.

The changelog renderer needs a Markdown link to the contributor's GitHub
profile.  Git only stores ``Name <email>`` locally, which may differ from
the GitHub profile name.  This module queries the GitHub REST API to
resolve the login, real name, and profile URL for each external commit.

Resolution strategy (with caching by login):

1. Parse the ``@users.noreply.github.com`` email — gives the login directly.
2. Fallback: GitHub Commits API ``GET /repos/{owner}/{repo}/commits/{sha}``
   returns ``author.login`` + ``author.html_url`` for the linked GitHub user.
3. Fetch ``GET /users/{login}`` for the real ``name`` (may be ``null``).

On any error, rate-limit, or unlinked commit the resolver leaves the
profile fields unset so the renderer falls back to plain-text output.
"""

from __future__ import annotations

import asyncio
from dataclasses import dataclass
import logging
import os
import re
from urllib.parse import urlparse

import httpx

from changelog_tool.common.models import ClassifiedCommit

logger = logging.getLogger(__name__)

ENV_GITHUB_TOKEN = 'CHANGELOG_GITHUB_TOKEN'

_GITHUB_API_BASE = 'https://api.github.com'

# Matches both noreply formats:
#   username@users.noreply.github.com
#   12345678+username@users.noreply.github.com
_NOREPLY_RE = re.compile(r'^(?:\d+\+)?([^@]+)@users\.noreply\.github\.com$')

# Concurrency cap for GitHub API requests.
_MAX_CONCURRENT_REQUESTS = 5


@dataclass(frozen=True, slots=True)
class GitHubProfile:
    """Resolved GitHub profile data for a contributor."""

    login: str
    name: str | None  # Real name from profile; may be None.
    profile_url: str


class GitHubProfileResolver:
    """Resolve GitHub profiles for external contributors via the REST API."""

    def __init__(
        self,
        github_url: str,
        token: str | None = None,
        timeout: float = 30.0,
    ) -> None:
        self._owner_repo = _extract_owner_repo(github_url)
        self._token = token or os.environ.get(ENV_GITHUB_TOKEN)
        self._timeout = timeout
        self._client: httpx.AsyncClient | None = None
        self._semaphore = asyncio.Semaphore(_MAX_CONCURRENT_REQUESTS)
        # Cache: login -> GitHubProfile (avoids redundant /users/{login} calls).
        self._profile_cache: dict[str, GitHubProfile | None] = {}

    async def resolve_profiles(self, commits: list[ClassifiedCommit]) -> None:
        """Populate ``github_*`` fields on each external commit.

        Commits whose profile cannot be resolved are left unchanged so the
        renderer falls back to plain-text output.
        """
        if not commits:
            return

        self._client = httpx.AsyncClient(
            base_url=_GITHUB_API_BASE,
            timeout=self._timeout,
            headers=self._default_headers(),
        )

        try:
            tasks = [self._resolve_one(commit) for commit in commits]
            await asyncio.gather(*tasks, return_exceptions=True)
        finally:
            await self._client.aclose()
            self._client = None

    def _default_headers(self) -> dict[str, str]:
        headers = {
            'Accept': 'application/vnd.github+json',
            'X-GitHub-Api-Version': '2022-11-28',
        }
        if self._token:
            headers['Authorization'] = f'Bearer {self._token}'
        return headers

    async def _resolve_one(self, commit: ClassifiedCommit) -> None:
        try:
            login = _extract_login_from_author(commit.author)
            if login is None and self._owner_repo is not None:
                login = await self._fetch_login_from_commit(commit.sha)

            if login is None:
                logger.debug('No GitHub login for commit %s', commit.sha[:8])
                return

            profile = await self._get_profile(login)
            if profile is None:
                return

            commit.github_login = profile.login
            commit.github_name = profile.name
            commit.github_profile_url = profile.profile_url
        except Exception:
            logger.exception('Failed to resolve GitHub profile for commit %s', commit.sha[:8])

    async def _fetch_login_from_commit(self, sha: str) -> str | None:
        """Use the Commits API to find the GitHub login linked to a commit."""
        if self._client is None or self._owner_repo is None:
            return None

        owner, repo = self._owner_repo
        url = f'/repos/{owner}/{repo}/commits/{sha}'

        async with self._semaphore:
            response = await self._client.get(url)

        if response.status_code == 404:
            logger.debug('Commit %s not found on GitHub', sha[:8])
            return None

        if response.status_code != 200:
            logger.warning(
                'GitHub commits API returned %d for %s',
                response.status_code,
                sha[:8],
            )
            return None

        data = response.json()
        author_obj = data.get('author')
        if not isinstance(author_obj, dict):
            # Commit email is not linked to any GitHub account.
            return None

        login = author_obj.get('login')
        return login if isinstance(login, str) and login else None

    async def _get_profile(self, login: str) -> GitHubProfile | None:
        """Fetch ``/users/{login}`` with caching by login."""
        if login in self._profile_cache:
            return self._profile_cache[login]

        if self._client is None:
            return None

        url = f'/users/{login}'

        async with self._semaphore:
            response = await self._client.get(url)

        if response.status_code == 404:
            logger.debug('GitHub user %s not found', login)
            self._profile_cache[login] = None
            return None

        if response.status_code != 200:
            logger.warning(
                'GitHub users API returned %d for %s',
                response.status_code,
                login,
            )
            self._profile_cache[login] = None
            return None

        data = response.json()
        name = data.get('name')
        html_url = data.get('html_url')

        if not isinstance(html_url, str) or not html_url:
            self._profile_cache[login] = None
            return None

        profile = GitHubProfile(
            login=login,
            name=name if isinstance(name, str) and name else None,
            profile_url=html_url,
        )
        self._profile_cache[login] = profile
        return profile


def _extract_owner_repo(github_url: str) -> tuple[str, str] | None:
    """Extract ``(owner, repo)`` from a GitHub repository URL."""
    parsed = urlparse(github_url)
    parts = [p for p in parsed.path.split('/') if p]
    if len(parts) < 2:
        return None
    return parts[0], parts[1]


def _extract_login_from_author(author: str) -> str | None:
    """Extract a GitHub login from a ``Name <email>`` git author string.

    Only ``@users.noreply.github.com`` emails are recognised — they
    directly encode the login.  Other emails require the Commits API.
    """
    email_match = re.search(r'<([^>]+)>', author)
    if not email_match:
        return None

    email = email_match.group(1).strip()
    noreply_match = _NOREPLY_RE.match(email)
    if not noreply_match:
        return None

    return noreply_match.group(1)
