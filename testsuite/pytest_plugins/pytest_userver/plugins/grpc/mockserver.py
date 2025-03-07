"""
Mocks for the gRPC servers.

@sa @ref scripts/docs/en/userver/tutorial/grpc_service.md
"""

# pylint: disable=no-member
import asyncio
import contextlib
import dataclasses
import functools
import inspect
from typing import Callable
from typing import Collection
from typing import Dict
from typing import List
from typing import Mapping
from typing import NoReturn
from typing import Optional
from typing import Tuple

import grpc
import pytest

from testsuite.utils import callinfo

MockDecorator = Callable[[Callable], callinfo.AsyncCallQueue]


# @cond


DEFAULT_PORT = 8091

USERVER_CONFIG_HOOKS = ['userver_config_grpc_mockserver']


class GrpcServiceMock:
    def __init__(self, servicer, adder: Callable, service_name: str, methods: Collection[str]):
        self._servicer = servicer
        self._adder = adder
        self._service_name = service_name
        self._known_methods = methods
        self._methods = {}

    @property
    def servicer(self):
        return self._servicer

    @property
    def service_name(self) -> str:
        return self._service_name

    def get(self, method_name, default, /):
        return self._methods.get(method_name, default)

    def add_to_server(self, server: grpc.aio.Server) -> None:
        self._adder(self._servicer, server)

    def reset_handlers(self):
        self._methods = {}

    @contextlib.contextmanager
    def mock(self):
        try:
            yield self.install_handler
        finally:
            self._methods = {}

    def install_handler(self, method_name: str, /) -> MockDecorator:
        def decorator(func):
            if method_name not in self._known_methods:
                raise RuntimeError(
                    f'Trying to mock unknown grpc method {method_name} in service {self._service_name}',
                )

            wrapped = callinfo.acallqueue(func)
            self._methods[method_name] = wrapped
            return wrapped

        return decorator


# @endcond


@pytest.fixture(scope='session')
def grpc_mockserver_endpoint(pytestconfig, get_free_port) -> str:
    """
    Returns the gRPC endpoint to start the mocking server that is set by
    command line `--grpc-mockserver-host` and `--grpc-mockserver-port` options.

    For port 0, picks some free port.

    Override this fixture to customize the endpoint used by gRPC mockserver.

    @snippet samples/grpc_service/tests/conftest.py  Prepare configs
    @ingroup userver_testsuite_fixtures
    """
    port = pytestconfig.option.grpc_mockserver_port
    if pytestconfig.option.service_wait or pytestconfig.option.service_disable:
        port = port or DEFAULT_PORT
    if port == 0:
        port = get_free_port()
    return f'{pytestconfig.option.grpc_mockserver_host}:{port}'


class GrpcMockserverSession:
    """
    Allows to install mocks that are kept active between tests, see
    @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver_session "grpc_mockserver_session".

    @warning This is a sharp knife, use with caution! For most use-cases, prefer
    @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver_new "grpc_mockserver_new" instead.
    """

    # @cond
    def __init__(self, *, _server: grpc.aio.Server) -> None:
        self._server = _server
        self._auto_service_mocks: Dict[type, GrpcServiceMock] = {}

    def get_auto_service_mock(self, servicer_class: type, /) -> GrpcServiceMock:
        existing_mock = self._auto_service_mocks.get(servicer_class, None)
        if existing_mock:
            return existing_mock
        new_mock = _create_servicer_mock(servicer_class)
        self._auto_service_mocks[servicer_class] = new_mock
        new_mock.add_to_server(self._server)
        return new_mock

    def reset_auto_service_mocks(self) -> None:
        for mock in self._auto_service_mocks.values():
            mock.reset_handlers()

    # @endcond

    @property
    def server(self) -> grpc.aio.Server:
        """
        The underlying @ref https://grpc.github.io/grpc/python/grpc_asyncio.html#grpc.aio.Server "grpc.aio.Server".
        """
        return self._server


@pytest.fixture(scope='session')
async def grpc_mockserver_session(grpc_mockserver_endpoint) -> GrpcMockserverSession:
    """
    Returns the gRPC mocking server.

    @warning This is a sharp knife, use with caution! For most use-cases, prefer
    @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver_new "grpc_mockserver_new" instead.

    @ingroup userver_testsuite_fixtures
    """
    server = grpc.aio.server()
    server.add_insecure_port(grpc_mockserver_endpoint)
    await server.start()

    try:
        yield GrpcMockserverSession(_server=server)
    finally:

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


class GrpcMockserver:
    """
    Allows to install mocks that are reset between tests, see
    @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver_new "grpc_mockserver_new".
    """

    # @cond
    def __init__(self, *, _mockserver_session: GrpcMockserverSession) -> None:
        self._mockserver_session = _mockserver_session

    # @endcond

    def __call__(self, servicer_method, /) -> MockDecorator:
        """
        Returns a decorator to mock the specified gRPC service method implementation.

        Example:

        @snippet samples/grpc_service/testsuite/test_grpc.py  Prepare modules
        @snippet samples/grpc_service/testsuite/test_grpc.py  grpc client test
        """
        servicer_class = _get_class_from_method(servicer_method)
        mock = self._mockserver_session.get_auto_service_mock(servicer_class)
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
            return self(getattr(servicer_class, method_name))

        return factory


@pytest.fixture
def grpc_mockserver_new(grpc_mockserver_session) -> GrpcMockserver:
    """
    Returns the gRPC mocking server.
    In order for gRPC clients in your service to work, mock handlers need to be installed for them using this fixture.

    Example:

    @snippet samples/grpc_service/testsuite/test_grpc.py  Prepare modules
    @snippet samples/grpc_service/testsuite/test_grpc.py  grpc client test

    Alternatively, you can create a shorthand for mocking frequently-used services:

    Mocks are only active within tests after their respective handler functions are created, not between tests.
    If you need the service needs the mock during startup, add the fixture that defines your mock to
    @ref pytest_userver.plugins.service.extra_client_deps "extra_client_deps".

    @ingroup userver_testsuite_fixtures
    """
    try:
        yield GrpcMockserver(_mockserver_session=grpc_mockserver_session)
    finally:
        grpc_mockserver_session.reset_auto_service_mocks()


@pytest.fixture(scope='session')
def userver_config_grpc_mockserver(grpc_mockserver_endpoint):
    """
    Returns a function that adjusts the static config for testsuite.
    Walks through config_vars *values* equal to `$grpc_mockserver`,
    and replaces them with @ref grpc_mockserver_endpoint.

    @ingroup userver_testsuite_fixtures
    """

    def patch_config(_config_yaml, config_vars):
        for name in config_vars:
            if config_vars[name] == '$grpc_mockserver':
                config_vars[name] = grpc_mockserver_endpoint

    return patch_config


@pytest.fixture(scope='session')
def grpc_mockserver(grpc_mockserver_session) -> grpc.aio.Server:
    """
    Returns the gRPC mocking server.

    @deprecated Please use @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver_new "grpc_mockserver_new"
    instead. Some time soon, `grpc_mockserver` will be removed.

    @ingroup userver_testsuite_fixtures
    """
    return grpc_mockserver_session.server


@pytest.fixture(scope='session')
def create_grpc_mock():
    """
    Creates the gRPC mock server for the provided type.

    @deprecated Please use @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver_new "grpc_mockserver_new"
    instead. Some time soon, `create_grpc_mock` will be removed.

    @ingroup userver_testsuite_fixtures
    """
    return _create_servicer_mock


# @cond


def pytest_addoption(parser):
    group = parser.getgroup('grpc-mockserver')
    group.addoption(
        '--grpc-mockserver-host',
        default='[::]',
        help='gRPC mockserver hostname, default is [::]',
    )
    group.addoption(
        '--grpc-mockserver-port',
        type=int,
        default=0,
        help='gRPC mockserver port, by default random port is used',
    )


def _raise_unimplemented_error(context: grpc.aio.ServicerContext) -> NoReturn:
    context.set_code(grpc.StatusCode.UNIMPLEMENTED)
    context.set_details('Method not implemented!')
    raise NotImplementedError('Method not implemented!')


def _create_servicer_mock(
    servicer_class: type,
    *,
    # TODO remove, these no longer do anything
    stream_method_names: Optional[List[str]] = None,
    before_call_hook: Optional[Callable[..., None]] = None,
) -> GrpcServiceMock:
    def wrap_grpc_method(name, default_method, is_stream):
        @functools.wraps(default_method)
        async def run_unary_response_method(self, request_or_stream, context: grpc.aio.ServicerContext):
            method = mock.get(name, None)
            if method is None:
                _raise_unimplemented_error(context)

            if before_call_hook is not None:
                before_call_hook(request_or_stream, context)
            return await method(request_or_stream, context)

        @functools.wraps(default_method)
        async def run_stream_response_method(self, request_or_stream, context: grpc.aio.ServicerContext):
            method = mock.get(name, None)
            if method is None:
                _raise_unimplemented_error(context)

            if before_call_hook is not None:
                before_call_hook(request_or_stream, context)
            async for response in await method(request_or_stream, context):
                yield response

        return run_stream_response_method if is_stream else run_unary_response_method

    if not servicer_class.__name__.endswith('Servicer'):
        raise ValueError(f'Expected *Servicer class, got: {servicer_class.__name__}')
    reflection = _reflect_servicer(servicer_class)

    methods = {}
    for attname, value in servicer_class.__dict__.items():
        if callable(value):
            methods[attname] = wrap_grpc_method(
                name=attname,
                default_method=value,
                is_stream=reflection[attname].response_streaming,
            )

    if reflection:
        service_name, _ = _split_rpc_name(next(iter(reflection.values())).name)
    else:
        service_name = 'UNKNOWN'
    adder = getattr(inspect.getmodule(servicer_class), f'add_{servicer_class.__name__}_to_server')

    mocked_servicer_class = type(
        f'Mock{servicer_class.__name__}',
        (servicer_class,),
        methods,
    )
    servicer = mocked_servicer_class()
    mock = GrpcServiceMock(servicer, adder, service_name, frozenset(methods))
    return mock


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
