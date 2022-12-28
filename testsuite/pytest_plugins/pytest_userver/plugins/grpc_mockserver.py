"""
Mocks for the gRPC servers.

@sa @ref md_en_userver_tutorial_grpc_service
"""

# pylint: disable=no-member
import asyncio
import contextlib
import functools

import grpc
import pytest

from testsuite.utils import callinfo

DEFAULT_PORT = 8091


class GrpcServiceMock:
    def __init__(self, servicer, methods):
        self.servicer = servicer
        self._known_methods = methods
        self._methods = {}

    def get(self, method, default):
        return self._methods.get(method, default)

    @contextlib.contextmanager
    def mock(self):
        try:
            yield self.install_handler
        finally:
            self._methods = {}

    def install_handler(self, method: str):
        def decorator(func):
            if method not in self._known_methods:
                raise RuntimeError(
                    f'Trying to mock unknown grpc method {method}',
                )

            wrapped = callinfo.acallqueue(func)
            self._methods[method] = wrapped
            return wrapped

        return decorator


@pytest.fixture(scope='session')
def _grpc_mockserver_endpoint(pytestconfig):
    port = pytestconfig.option.grpc_mockserver_port
    if pytestconfig.option.service_wait or pytestconfig.option.service_disable:
        port = port or DEFAULT_PORT
    return f'{pytestconfig.option.grpc_mockserver_host}:{port}'


@pytest.fixture(scope='session')
def grpc_mockserver_endpoint(pytestconfig, _grpc_mockserver_and_port) -> str:
    """
    Returns the gRPC endpoint to start the mocking server that is set by
    command line `--grpc-mockserver-host` and `--grpc-mockserver-port` options.

    Override this fixture to change the way the gRPC endpoint
    is detected by the testsuite.

    @snippet samples/grpc_service/tests/conftest.py  Prepare configs
    @ingroup userver_testsuite_fixtures
    """
    _, port = _grpc_mockserver_and_port
    return f'{pytestconfig.option.grpc_mockserver_host}:{port}'


@pytest.fixture(scope='session')
async def _grpc_mockserver_and_port(_grpc_mockserver_endpoint):
    server = grpc.aio.server()
    port = server.add_insecure_port(_grpc_mockserver_endpoint)
    return server, port


@pytest.fixture(scope='session')
async def grpc_mockserver(_grpc_mockserver_and_port):
    """
    Returns the gRPC mocking server.

    Override this fixture to change the way the gRPC
    mocking server is started by the testsuite.

    @snippet samples/grpc_service/tests/conftest.py  Prepare server mock
    @ingroup userver_testsuite_fixtures
    """
    server, _ = _grpc_mockserver_and_port
    server_task = asyncio.create_task(server.start())

    try:
        yield server
    finally:
        await server.stop(grace=None)
        await server.wait_for_termination()
        await server_task


@pytest.fixture(scope='session')
def create_grpc_mock():
    """
    Creates the gRPC mock server for the provided type.

    @snippet samples/grpc_service/tests/conftest.py  Prepare server mock
    @ingroup userver_testsuite_fixtures
    """
    return _create_servicer_mock


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


def _create_servicer_mock(servicer_class):
    def wrap_grpc_method(name, default_method):
        @functools.wraps(default_method)
        async def run_method(self, *args, **kwargs):
            method = mock.get(name, default_method)
            return await method(*args, **kwargs)

        return run_method

    methods = {}
    for attname, value in servicer_class.__dict__.items():
        if callable(value):
            methods[attname] = wrap_grpc_method(attname, value)

    mocked_servicer_class = type(
        f'Mock{servicer_class.__name__}', (servicer_class,), methods,
    )
    servicer = mocked_servicer_class()
    mock = GrpcServiceMock(servicer, frozenset(methods))
    return mock
