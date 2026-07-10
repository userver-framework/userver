class BaseError(Exception):
    def __init__(self, message: str):
        super().__init__('\n'.join(line.strip() for line in message.strip().splitlines()))


class ClientError(BaseError):
    pass


class RuntimeError(BaseError):
    pass


class UnsupportedError(BaseError):
    pass


class UnexpectedError(BaseError):
    pass
