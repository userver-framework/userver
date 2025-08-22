import abc


class Middleware(abc.ABC):
    """
    Per-operation middleware.
    It generates a single struct member for `Request` type.
    If we want to represent the structure as a jinja template,
    it will look something like that:

        struct Request {
            {{ middleware.request_member_cpp_type }} {{ middleware.request_member_name }};
        };

        void SerializeRequest(
            const Request& request,
            const std::string& base_url,
            USERVER_NAMESPACE::clients::http::Request& http_request
        )
        {
            openapi::ParameterSinkHttpClient sink(
                http_request,
                base_url + "{{ op.path }}"
            );
            ...
            {{ middleware.write_member_command('request', 'sink', 'http_request') }}
        }
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

    A plugin is created per-parser.
    """

    @property
    @abc.abstractmethod
    def field(self) -> str:
        """
        Returns `x-usrv-middleware` property name that is responsible for
        middleware activation and parameters storage.
        """
        pass

    @abc.abstractmethod
    def create(self, args: dict) -> Middleware:
        """
        The method is called every time a HTTP operation with
        "x-usrv-middleware." + field property is discovered.
        It creates a per-operation `Middleware` object.
        """
        pass
