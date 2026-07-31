"""Custom exception hierarchy for LLM-related errors."""


class LLMError(Exception):
    """Critical LLM error that should not be retried."""


class LLMTransientError(LLMError):
    """Transient LLM error that may succeed on retry."""
