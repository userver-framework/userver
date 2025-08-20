import abc


class Middleware(abc.ABC):
    """
    Per-operation middleware.
    It generates a single struct member for `Request` type.
    """

    @property
    @abc.abstractmethod
    def request_member_name(self) -> str:
        """
        Generated `Request` member name for middleware data.
        """
        pass

    @property
    @abc.abstractmethod
    def request_member_cpp_type(self) -> str:
        """
        C++ type of `request_member_name`.
        """
        pass

    @property
    @abc.abstractmethod
    def requests_hpp_includes(self) -> list[str]:
        """
        C++ include path used to access `request_member_cpp_type`.
        """
        pass

    @abc.abstractmethod
    def write_member_command(
        self,
        request: str,
        sink: str,
        http_request: str,
    ) -> str:
        """
        C++ code that will be called as part of `Request` serialization.

        request - variable name of `Request` type
        sink - variable name of `ParameterSinkHttpClient` type
        http_request - variable name of `clients::http::Request` type
        """
        pass


class MiddlewarePlugin(abc.ABC):
    """
    Implement `MiddlewarePlugin` if you want to extend OpenAPI schema
    with your middleware. The schema is extended with
    `x-usrv-middleware.` + MiddlewarePlugin.field object member.
    Each operation with the middleware member triggers `create`.

    A plugin is created per-parser.
    """

    @property
    @abc.abstractmethod
    def field(self) -> str:
        pass

    @abc.abstractmethod
    def create(self, args: dict) -> Middleware:
        pass
