import pydantic

class LLMConfig(pydantic.BaseModel):
    target_rps: float = 5.0
    retries: int = 3
    max_commits_per_batch: int = 10
    max_user_prompt_length: int = 8000
    include_diff: bool = True
    truncate_diff: bool = True
    max_concurrent_requests: int = 5
