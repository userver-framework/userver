import pydantic

class LLMConfig(pydantic.BaseModel):
    target_rps: float = 5.0
    retries: int = 3
