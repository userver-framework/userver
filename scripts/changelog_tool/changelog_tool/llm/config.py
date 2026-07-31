"""Configuration model for the LLM subsystem."""

import pydantic


class LLMConfig(pydantic.BaseModel):
    target_rps: float = 5.0
    retries: int = 3
    max_commits_per_batch: int = 10
    max_user_prompt_length: int = 8000
    max_concurrent_requests: int = 5

    max_rounds: int = 5
    round_delay_seconds: int = 300
    backoff_max_seconds: float = 60.0
