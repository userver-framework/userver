"""
Mocks for the gRPC servers.

@sa @ref scripts/docs/en/userver/tutorial/grpc_service.md
@sa @ref pytest_userver.plugins.grpc.mockserver
"""

from __future__ import annotations

import asyncio
import contextlib
import dataclasses
import functools
import inspect
from typing import Any
from typing import Callable
from typing import Collection
from typing import Dict
from typing import Mapping
from typing import NoReturn
from typing import Tuple

import grpc
from typing_extensions import Final  # TODO use typing in Python 3.8

import testsuite.utils.callinfo

Handler = Callable[[Any, grpc.aio.ServicerContext], Any]
MockDecorator = Callable[[Handler], testsuite.utils.callinfo.AsyncCallQueue]

_ERROR_CODE_KEY = 'x-testsuite-error-code'


class MockedError(Exception):
    """
    @brief Base class for mocked exceptions.
    @alias pytest_userver.grpc.MockedError
    """

    ERROR_CODE = 'unknown'


class TimeoutError(MockedError):  # pylint: disable=redefined-builtin
    """
    @brief When thrown from a mocked handler, leads to `ugrpc::client::RpcInterruptedError`.
    @alias pytest_userver.grpc.TimeoutError

    Example:
    @snippet grpc/functional_tests/middleware_client/tests/test_mocked_fail.py Mocked timeout failure
    """

    ERROR_CODE = 'timeout'


class NetworkError(MockedError):
    """
    @brief When thrown from a mocked handler, leads to `ugrpc::client::RpcInterruptedError`.
    @alias pytest_userver.grpc.NetworkError

    Example:
    @snippet grpc/functional_tests/middleware_client/tests/test_mocked_fail.py Mocked network failure
    """

    ERROR_CODE = 'network'


class MockserverSession:
    """
    Allows to install mocks that are kept active between tests, see
    @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver_session "grpc_mockserver_session".

    @warning This is a sharp knife, use with caution! For most use-cases, prefer
    @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver_new "grpc_mockserver_new" instead.
    """

    def __init__(self, *, server: grpc.aio.Server, experimental: bool = False) -> None:
        """
        @warning This initializer is an **experimental API**, likely to break in the future. Consider using
        @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver_session "grpc_mockserver_session" instead.

        Initializes `MockserverSession`. Takes ownership of `server`.
        To properly start and stop the server, use `MockserverSession` as an async contextmanager:

        @code{.py}
        async with pytest_userver.grpc.mockserver.MockserverSession(server=server) as mockserver:
            # ...
        @endcode

        @note `MockserverSession` is usually obtained from
        @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver_session "grpc_mockserver_session".
        """
        assert experimental
        self._server = server
        self._auto_service_mocks: Dict[type, _ServiceMock] = {}

    def _get_auto_service_mock(self, servicer_class: type, /) -> _ServiceMock:
        existing_mock = self._auto_service_mocks.get(servicer_class)
        if existing_mock:
            return existing_mock
        new_mock = _create_servicer_mock(servicer_class)
        self._auto_service_mocks[servicer_class] = new_mock
        new_mock.add_to_server(self._server)
        return new_mock

    def reset_mocks(self) -> None:
        """
        Removes all mocks for this mockserver, regardless of the way they were installed.
        """
        for mock in self._auto_service_mocks.values():
            mock.reset_handlers()

    @property
    def server(self) -> grpc.aio.Server:
        """
        The underlying @ref https://grpc.github.io/grpc/python/grpc_asyncio.html#grpc.aio.Server "grpc.aio.Server".
        """
        return self._server

    async def start(self) -> None:
        """
        Starts the server.
        @note Prefer starting mockserver using the async contextmanager syntax if possible.
        """
        await self._server.start()

    async def stop(self) -> None:
        """
        Stops the server properly. Prefer this method to stopping `server` manually.
        """
        await _stop_server(self._server)

    async def __aenter__(self) -> MockserverSession:
        await self.start()
        return self

    async def __aexit__(self, exc_type, exc_val, exc_tb) -> None:
        await self.stop()


class Mockserver:
    """
    Allows to install mocks that are reset between tests, see
    @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver_new "grpc_mockserver_new".
    """

    def __init__(self, *, mockserver_session: MockserverSession, experimental: bool = False) -> None:
        """
        @warning This initializer is an **experimental API**, likely to break in the future. Consider using
        @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver_new "grpc_mockserver_new" instead.

        Initializes Mockserver.

        @note `Mockserver` is usually obtained from
        @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver_new "grpc_mockserver_new".
        """
        assert experimental
        self._mockserver_session = mockserver_session

    def __call__(self, servicer_method, /) -> MockDecorator:
        """
        Returns a decorator to mock the specified gRPC service method implementation.

        Example:

        @snippet samples/grpc_service/testsuite/test_grpc.py  Prepare modules
        @snippet samples/grpc_service/testsuite/test_grpc.py  grpc client test
        """
        servicer_class = _get_class_from_method(servicer_method)
        mock = self._mockserver_session._get_auto_service_mock(servicer_class)  # pylint: disable=protected-access
        return mock.install_handler(servicer_method.__name__)

    def mock_factory(self, servicer_class, /) -> Callable[[str], MockDecorator]:
        """
        Allows to create a fixture as a shorthand for mocking methods of the specified gRPC service.

        Example:

        @snippet grpc/functional_tests/metrics/tests/conftest.py  Prepare modules
        @snippet grpc/functional_tests/metrics/tests/conftest.py  Prepare server mock
        @snippet grpc/functional_tests/metrics/tests/test_metrics.py  grpc client test
        """

        def factory(method_name):
            method = getattr(servicer_class, method_name, None)
            if method is None:
                raise ValueError(f'No method "{method_name}" in servicer class "{servicer_class.__name__}"')
            return self(method)

        _check_is_servicer_class(servicer_class)
        return factory


# @cond


class _ServiceMock:
    def __init__(
        self,
        *,
        servicer: object,
        adder: Callable[[object, grpc.aio.Server], None],
        service_name: str,
        known_methods: Collection[str],
        mocked_methods: Dict[str, Handler],
    ) -> None:
        self._servicer = servicer
        self._adder = adder
        self._service_name = service_name
        self._known_methods = known_methods
        # Note: methods of `servicer` look into `mocked_methods` for implementations.
        # (Specifically into the instance our constructor is given.)
        self._methods: Final[Dict[str, Handler]] = mocked_methods

    @property
    def servicer(self) -> object:
        return self._servicer

    @property
    def service_name(self) -> str:
        return self._service_name

    def get(self, method_name, default, /):
        return self._methods.get(method_name, default)

    def add_to_server(self, server: grpc.aio.Server) -> None:
        self._adder(self._servicer, server)

    def reset_handlers(self) -> None:
        self._methods.clear()

    @contextlib.contextmanager
    def mock(self):
        try:
            yield self.install_handler
        finally:
            self._methods.clear()

    def install_handler(self, method_name: str, /) -> MockDecorator:
        def decorator(func):
            if method_name not in self._known_methods:
                raise RuntimeError(
                    f'Trying to mock unknown grpc method {method_name} in service {self._service_name}. '
                    f'Known methods are: {", ".join(self._known_methods)}',
                )

            wrapped = testsuite.utils.callinfo.acallqueue(func)
            self._methods[method_name] = wrapped
            return wrapped

        return decorator


async def _stop_server(server: grpc.aio.Server, /) -> None:
    async def stop_server():
        await server.stop(grace=None)
        await server.wait_for_termination()

    stop_server_task = asyncio.shield(asyncio.create_task(stop_server()))
    # asyncio.shield does not protect our await from cancellations, and we
    # really need to wait for the server stopping before continuing.
    try:
        await stop_server_task
    except asyncio.CancelledError:
        await stop_server_task
        # Propagate cancellation when we are done
        raise


def _guess_servicer(module) -> str:
    guesses = [member for member in dir(module) if str(member).endswith('Servicer')]
    assert len(guesses) == 1, f"Don't know what servicer to choose: {guesses}"
    return str(guesses[0])


def _raise_unimplemented_error(context: grpc.aio.ServicerContext, full_rpc_name: str) -> NoReturn:
    message = f'Trying to call grpc_mockserver method "{full_rpc_name}", which is not mocked'
    context.set_code(grpc.StatusCode.UNIMPLEMENTED)
    context.set_details(message)
    raise NotImplementedError(message)


@contextlib.asynccontextmanager
async def _transform_mocked_error(context: grpc.aio.ServicerContext):
    try:
        yield
    except MockedError as exc:
        await context.abort(code=grpc.StatusCode.UNKNOWN, trailing_metadata=((_ERROR_CODE_KEY, exc.ERROR_CODE),))


def _wrap_grpc_method(
    python_method_name: str,
    full_rpc_name: str,
    default_unimplemented_method: Handler,
    response_streaming: bool,
    mocked_methods: Dict[str, Handler],
):
    if response_streaming:

        @functools.wraps(default_unimplemented_method)
        async def run_stream_response_method(self, request_or_stream, context: grpc.aio.ServicerContext):
            method = mocked_methods.get(python_method_name)
            if method is None:
                _raise_unimplemented_error(context, full_rpc_name)

            async with _transform_mocked_error(context):
                async for response in await method(request_or_stream, context):
                    yield response

        return run_stream_response_method
    else:

        @functools.wraps(default_unimplemented_method)
        async def run_unary_response_method(self, request_or_stream, context: grpc.aio.ServicerContext):
            method = mocked_methods.get(python_method_name)
            if method is None:
                _raise_unimplemented_error(context, full_rpc_name)

            async with _transform_mocked_error(context):
                response = method(request_or_stream, context)
                if inspect.isawaitable(response):
                    return await response
                else:
                    return response

        return run_unary_response_method


def _check_is_servicer_class(servicer_class: type) -> None:
    if not isinstance(servicer_class, type):
        raise ValueError(f'Expected *Servicer class (type), got: {servicer_class} ({type(servicer_class)})')
    if not servicer_class.__name__.endswith('Servicer'):
        raise ValueError(f'Expected *Servicer class, got: {servicer_class}')


def _create_servicer_mock(servicer_class: type, /) -> _ServiceMock:
    _check_is_servicer_class(servicer_class)
    reflection = _reflect_servicer(servicer_class)

    if reflection:
        service_name, _ = _split_rpc_name(next(iter(reflection.values())).name)
    else:
        service_name = 'UNKNOWN'
    adder = getattr(inspect.getmodule(servicer_class), f'add_{servicer_class.__name__}_to_server')

    mocked_methods: Dict[str, Handler] = {}

    methods = {}
    for attname, value in servicer_class.__dict__.items():
        if callable(value) and not attname.startswith('_'):
            methods[attname] = _wrap_grpc_method(
                python_method_name=attname,
                full_rpc_name=reflection[attname].name,
                default_unimplemented_method=value,
                response_streaming=reflection[attname].response_streaming,
                mocked_methods=mocked_methods,
            )
    mocked_servicer_class = type(
        f'Mock{servicer_class.__name__}',
        (servicer_class,),
        methods,
    )
    servicer = mocked_servicer_class()

    return _ServiceMock(
        servicer=servicer,
        adder=adder,
        service_name=service_name,
        known_methods=frozenset(methods),
        mocked_methods=mocked_methods,
    )


@dataclasses.dataclass(frozen=True)
class _MethodInfo:
    name: str  # in the format "/with.package.ServiceName/MethodName"
    request_streaming: bool
    response_streaming: bool


class _FakeChannel:
    def unary_unary(self, name, *args, **kwargs):
        return _MethodInfo(name=name, request_streaming=False, response_streaming=False)

    def unary_stream(self, name, *args, **kwargs):
        return _MethodInfo(name=name, request_streaming=False, response_streaming=True)

    def stream_unary(self, name, *args, **kwargs):
        return _MethodInfo(name=name, request_streaming=True, response_streaming=False)

    def stream_stream(self, name, *args, **kwargs):
        return _MethodInfo(name=name, request_streaming=True, response_streaming=True)


def _reflect_servicer(servicer_class: type) -> Mapping[str, _MethodInfo]:
    service_name = servicer_class.__name__.removesuffix('Servicer')
    stub_class = getattr(inspect.getmodule(servicer_class), f'{service_name}Stub')
    return _reflect_stub(stub_class)


def _reflect_stub(stub_class: type) -> Mapping[str, _MethodInfo]:
    # HACK: relying on the implementation of generated *Stub classes.
    return stub_class(_FakeChannel()).__dict__


def _split_rpc_name(rpc_name: str) -> Tuple[str, str]:
    normalized = rpc_name.removeprefix('/')
    results = normalized.split('/')
    if len(results) != 2:
        raise ValueError(
            f'Invalid RPC name: "{rpc_name}". RPC name must be of the form "with.package.ServiceName/MethodName"'
        )
    return results


def _get_class_from_method(method) -> type:
    # https://stackoverflow.com/a/54597033/5173839
    assert inspect.isfunction(method), 'Expected an unbound method: foo(ClassName.MethodName)'
    class_name = method.__qualname__.split('.<locals>', 1)[0].rsplit('.', 1)[0]
    try:
        cls = getattr(inspect.getmodule(method), class_name)
    except AttributeError:
        cls = method.__globals__.get(class_name)
    assert isinstance(cls, type)
    return cls


# @endcond
