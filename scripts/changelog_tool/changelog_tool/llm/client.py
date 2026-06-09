import os
import asyncio
from abc import ABC, abstractmethod

import aiohttp
from aiolimiter import AsyncLimiter

from changelog_tool.llm.config import LLMConfig
from changelog_tool.llm.exceptions import LLMError, LLMTransientError


class BaseLLMClient(ABC):
    @abstractmethod
    async def generate(self, prompt: str) -> str:
        """
        Асинхронно отправляет текстовый промпт в LLM и возвращает текстовый ответ.
        Может выбрасывать LLMError или LLMTransientError.
        """
        pass

    @abstractmethod
    async def close(self):
        """Закрывает ресурсы клиента."""
        pass


class HttpLLMClient(BaseLLMClient):
    def __init__(self, config: LLMConfig):
        self.url = os.environ.get("CHANGELOG_LLM_URL")
        api_key = os.environ.get("CHANGELOG_LLM_API_KEY")
        oauth_key = os.environ.get("CHANGELOG_LLM_OAUTH_KEY")
        self.retries = config.retries
        
        if not self.url:
            raise RuntimeError("Missing required environment variable: CHANGELOG_LLM_URL")
            
        if api_key:
            auth_header = f"Bearer {api_key}"
        elif oauth_key:
            auth_header = f"OAuth {oauth_key}"
        else:
            raise RuntimeError("Missing required environment variable: either CHANGELOG_LLM_API_KEY or CHANGELOG_LLM_OAUTH_KEY must be set")
            
        self.limiter = AsyncLimiter(config.target_rps, 1)
        self.session = aiohttp.ClientSession(
            headers={"Authorization": auth_header}
        )

    async def generate(self, prompt: str) -> str:
        last_error = None
        
        for attempt in range(self.retries + 1):
            try:
                async with self.limiter:
                    async with self.session.post(self.url, json={"prompt": prompt}) as response:
                        if response.status == 200:
                            data = await response.json()
                            return data.get("response", "")
                            
                        if response.status in (400, 401, 403, 404):
                            text = await response.text()
                            raise LLMError(f"Critical LLM error: {response.status} - {text}")
                            
                        if response.status == 429:
                            retry_after = response.headers.get("Retry-After")
                            if retry_after and retry_after.isdigit():
                                wait_time = float(retry_after)
                            else:
                                wait_time = 2 ** attempt
                            
                            last_error = f"429 Too Many Requests. Waiting {wait_time}s"
                            await asyncio.sleep(wait_time)
                            continue
                            
                        if response.status >= 500:
                            last_error = f"Server error {response.status}"
                            await asyncio.sleep(2 ** attempt)
                            continue
                            
                        # Неизвестный статус
                        text = await response.text()
                        raise LLMError(f"Unexpected status {response.status}: {text}")
                        
            except aiohttp.ClientError as e:
                last_error = f"Client error: {e}"
                await asyncio.sleep(2 ** attempt)
                continue
                
        raise LLMTransientError(f"Max retries ({self.retries}) exceeded. Last error: {last_error}")

    async def close(self):
        await self.session.close()
