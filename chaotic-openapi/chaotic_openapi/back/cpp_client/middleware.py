import abc


class Middleware(abc.ABC):
    @property
    @abc.abstractmethod
    def request_member_name(self) -> str:
        pass

    @property
    @abc.abstractmethod
    def request_member_cpp_type(self) -> str:
        pass

    @property
    @abc.abstractmethod
    def requests_hpp_includes(self) -> list[str]:
        pass

    @abc.abstractmethod
    def write_member_command(
        self,
        request: str,
        sink: str,
        http_request: str,
    ) -> str:
        pass


class MiddlewarePlugin(abc.ABC):
    @property
    @abc.abstractmethod
    def field(self) -> str:
        pass

    @abc.abstractmethod
    def create(self, args: dict) -> Middleware:
        pass
