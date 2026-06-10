import os
import asyncio
from abc import ABC, abstractmethod

import httpx
import openai
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
        self.model = os.environ.get("CHANGELOG_LLM_MODEL")
        self.retries = config.retries
        
        if not self.url:
            raise RuntimeError("Missing required environment variable: CHANGELOG_LLM_URL")
            
        if api_key:
            auth_header = f"Bearer {api_key}"
        elif oauth_key:
            auth_header = f"Oauth {oauth_key}"
        else:
            raise RuntimeError("Missing required environment variable: either CHANGELOG_LLM_API_KEY or CHANGELOG_LLM_OAUTH_KEY must be set")
            
        self.limiter = AsyncLimiter(config.target_rps, 1)
        
        http_client = httpx.AsyncClient(verify=False)
        
        self.client = openai.AsyncOpenAI(
            base_url=self.url,
            api_key=api_key or oauth_key or "dummy",
            default_headers={"Authorization": auth_header},
            max_retries=0,
            http_client=http_client,
        )

    async def generate(self, prompt: str) -> str:
        last_error = None
        
        for attempt in range(self.retries + 1):
            try:
                async with self.limiter:
                    if attempt > 0:
                        print(f"  Retrying ({attempt}/{self.retries})...")
                    
                    response = await self.client.chat.completions.create(
                        model=self.model,
                        messages=[{"role": "user", "content": prompt}],
                    )
                    
                    # Handle non-standard API response format
                    # Some APIs return the actual data in a 'response' dict attribute
                    if hasattr(response, 'response') and response.response:
                        response_data = response.response
                        if isinstance(response_data, dict) and 'choices' in response_data:
                            choices = response_data['choices']
                            if choices and len(choices) > 0:
                                first_choice = choices[0]
                                if 'message' in first_choice and 'content' in first_choice['message']:
                                    content = first_choice['message']['content']
                                    return content or ""
                    
                    # Standard OpenAI response format
                    if not response:
                        raise ValueError("LLM returned None response")
                    
                    if not hasattr(response, 'choices') or not response.choices:
                        raise ValueError(f"LLM response has no choices. Response: {response}")
                    
                    if len(response.choices) == 0:
                        raise ValueError(f"LLM returned empty choices list. Response: {response}")
                    
                    first_choice = response.choices[0]
                    if not hasattr(first_choice, 'message'):
                        raise ValueError(f"First choice has no message attribute. Choice: {first_choice}")
                    
                    message = first_choice.message
                    if not hasattr(message, 'content'):
                        raise ValueError(f"Message has no content attribute. Message: {message}")
                    
                    content = message.content
                    return content or ""
                    
            except openai.RateLimitError as e:
                last_error = f"Rate limit: {e}"
                print(f"  Rate limit hit, waiting...")
                await asyncio.sleep(2 ** attempt)
                continue
            except openai.APIStatusError as e:
                if e.status_code in (400, 401, 403, 404):
                    raise LLMError(f"Critical LLM error: {e.status_code} - {e.message}")
                if e.status_code >= 500:
                    last_error = f"Server error {e.status_code}"
                    print(f"  Server error, retrying...")
                    await asyncio.sleep(2 ** attempt)
                    continue
                raise LLMError(f"Unexpected status {e.status_code}: {e.message}")
            except openai.APIError as e:
                last_error = f"API error: {e}"
                print(f"  API error, retrying...")
                await asyncio.sleep(2 ** attempt)
                continue
            except Exception as e:
                last_error = f"Client error: {e}"
                print(f"  Error, retrying...")
                await asyncio.sleep(2 ** attempt)
                continue
                
        raise LLMTransientError(f"Max retries ({self.retries}) exceeded. Last error: {last_error}")

    async def close(self):
        await self.client.close()
