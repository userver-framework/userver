class LLMError(Exception):
    """Critical LLM error (e.g., invalid request format, 400 Bad Request)."""
    pass

class LLMTransientError(LLMError):
    """Transient LLM error (e.g., 500, 503, timeout or retries exhausted)."""
    pass
