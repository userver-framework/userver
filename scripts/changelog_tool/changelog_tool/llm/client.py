"""LLM HTTP client with retry, rate limiting, and concurrency control."""

from abc import ABC
from abc import abstractmethod
import asyncio
import logging
import os
import random

from aiolimiter import AsyncLimiter
import httpx
import openai

from changelog_tool.llm.config import LLMConfig
from changelog_tool.llm.exceptions import LLMError
from changelog_tool.llm.exceptions import LLMTransientError

logger = logging.getLogger(__name__)

ENV_LLM_URL = 'CHANGELOG_LLM_URL'
ENV_LLM_API_KEY = 'CHANGELOG_LLM_API_KEY'
ENV_LLM_OAUTH_KEY = 'CHANGELOG_LLM_OAUTH_KEY'
ENV_LLM_MODEL = 'CHANGELOG_LLM_MODEL'
ENV_LLM_CA_BUNDLE = 'CHANGELOG_LLM_CA_BUNDLE'

# Well-known system CA bundle locations, checked when ENV_LLM_CA_BUNDLE is
# not set. Unlike certifi, these include internal corporate root CAs.
_SYSTEM_CA_BUNDLE_CANDIDATES = (
    '/etc/ssl/certs/ca-certificates.crt',  # Debian/Ubuntu
    '/etc/pki/tls/certs/ca-bundle.crt',  # RHEL/Fedora/CentOS
    '/etc/ssl/ca-bundle.pem',  # openSUSE
)

_CRITICAL_STATUS_CODES = frozenset({400, 401, 403, 404})


def _resolve_ca_bundle() -> str | bool:
    override = os.environ.get(ENV_LLM_CA_BUNDLE)
    if override:
        return override

    for candidate in _SYSTEM_CA_BUNDLE_CANDIDATES:
        if os.path.exists(candidate):
            return candidate

    return True


def _parse_retry_after(headers: object) -> float | None:
    if headers is None:
        return None
    try:
        value = headers.get('retry-after')  # type: ignore[attr-defined]
    except AttributeError:
        return None
    if not value:
        return None
    try:
        return max(0.0, float(value))
    except (TypeError, ValueError):
        return None


def _extract_retry_after(exc: Exception) -> float | None:
    response = getattr(exc, 'response', None)
    headers = getattr(response, 'headers', None) if response is not None else None
    return _parse_retry_after(headers)


class BaseLLMClient(ABC):
    """Abstract base class for LLM clients."""

    @abstractmethod
    async def generate(self, system_prompt: str, user_prompt: str) -> str: ...

    @abstractmethod
    async def close(self) -> None: ...


class HttpLLMClient(BaseLLMClient):
    """HTTP-based LLM client using the OpenAI-compatible API."""

    def __init__(self, config: LLMConfig) -> None:
        self._config = config
        self._url = os.environ.get(ENV_LLM_URL)
        api_key = os.environ.get(ENV_LLM_API_KEY)
        oauth_key = os.environ.get(ENV_LLM_OAUTH_KEY)
        self._model = os.environ.get(ENV_LLM_MODEL)
        self._retries = config.retries
        self._backoff_max_seconds = config.backoff_max_seconds

        if not self._url:
            raise LLMError(f'Missing required environment variable: {ENV_LLM_URL}')

        if api_key:
            auth_header = f'Bearer {api_key}'
        elif oauth_key:
            auth_header = f'Oauth {oauth_key}'
        else:
            raise LLMError(
                f'Missing required environment variable: either {ENV_LLM_API_KEY} or {ENV_LLM_OAUTH_KEY} must be set'
            )

        if not self._model:
            raise LLMError(f'Missing required environment variable: {ENV_LLM_MODEL}')

        self._limiter = AsyncLimiter(config.target_rps, 1)
        self._semaphore = asyncio.Semaphore(config.max_concurrent_requests)

        http_client = httpx.AsyncClient(verify=_resolve_ca_bundle())

        self._client = openai.AsyncOpenAI(
            base_url=self._url,
            api_key=api_key or oauth_key or 'dummy',
            default_headers={'Authorization': auth_header},
            max_retries=0,
            http_client=http_client,
        )

    def _compute_backoff(self, attempt: int, retry_after: float | None = None) -> float:
        if retry_after is not None:
            delay = retry_after
        else:
            delay = float(2**attempt)

        delay = min(delay, self._backoff_max_seconds)

        delay = random.uniform(0, delay) if delay > 0 else 0.0

        return delay

    async def generate(self, system_prompt: str, user_prompt: str) -> str:
        last_error: str | None = None

        async with self._semaphore:
            for attempt in range(self._retries + 1):
                try:
                    async with self._limiter:
                        if attempt > 0:
                            logger.info('Retrying (%d/%d)...', attempt, self._retries)

                        response = await self._client.chat.completions.create(
                            model=self._model,
                            messages=[
                                {'role': 'system', 'content': system_prompt},
                                {'role': 'user', 'content': user_prompt},
                            ],
                        )

                    return _extract_response_content(response)

                except openai.RateLimitError as e:
                    last_error = f'Rate limit: {e}'
                    retry_after = _extract_retry_after(e)
                    delay = self._compute_backoff(attempt, retry_after)
                    logger.warning('Rate limit hit, backing off for %.1fs...', delay)
                    await asyncio.sleep(delay)
                except openai.APIStatusError as e:
                    if e.status_code in _CRITICAL_STATUS_CODES:
                        raise LLMError(f'Critical LLM error: {e.status_code} - {e.message}')
                    if e.status_code >= 500:
                        last_error = f'Server error {e.status_code}'
                        delay = self._compute_backoff(attempt)
                        logger.warning('Server error %d, retrying in %.1fs...', e.status_code, delay)
                        await asyncio.sleep(delay)
                    else:
                        raise LLMError(f'Unexpected status {e.status_code}: {e.message}')
                except openai.APIError as e:
                    last_error = f'API error: {e}'
                    delay = self._compute_backoff(attempt)
                    logger.warning('API error, retrying in %.1fs: %s', delay, e)
                    await asyncio.sleep(delay)
                except (httpx.HTTPError, httpx.RequestError, asyncio.TimeoutError) as e:
                    last_error = f'Network error: {e}'
                    delay = self._compute_backoff(attempt)
                    logger.warning('Network error, retrying in %.1fs: %s', delay, e)
                    await asyncio.sleep(delay)

        raise LLMTransientError(f'Max retries ({self._retries}) exceeded. Last error: {last_error}')

    async def close(self) -> None:
        await self._client.close()


def _extract_response_content(response: object) -> str:
    if not response:
        raise LLMError('LLM returned None response')

    choices = getattr(response, 'choices', None)
    if not choices:
        raise LLMError(f'LLM response has no choices. Response: {response}')

    first_choice = choices[0]
    message = getattr(first_choice, 'message', None)
    if message is None:
        raise LLMError(f'First choice has no message attribute. Choice: {first_choice}')

    content = getattr(message, 'content', None)
    if content is None:
        raise LLMError(f'Message has no content attribute. Message: {message}')

    return content or ''
