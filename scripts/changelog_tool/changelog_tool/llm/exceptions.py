class LLMError(Exception):
    """Критическая ошибка LLM (например, неверный формат запроса, 400 Bad Request)."""
    pass

class LLMTransientError(LLMError):
    """Временная ошибка LLM (например, 500, 503, таймаут или исчерпаны попытки ретраев)."""
    pass
