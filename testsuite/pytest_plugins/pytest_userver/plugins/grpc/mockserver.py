"""
Mocks for the gRPC servers.

@sa @ref scripts/docs/en/userver/tutorial/grpc_service.md
"""

import grpc
import pytest

import pytest_userver.grpc

# @cond


DEFAULT_PORT = 8091

USERVER_CONFIG_HOOKS = ['userver_config_grpc_mockserver']


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


@pytest.fixture(scope='session')
async def grpc_mockserver_session(grpc_mockserver_endpoint) -> pytest_userver.grpc.MockserverSession:
    """
    Returns the gRPC mocking server.

    @warning This is a sharp knife, use with caution! For most use-cases, prefer
    @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver_new "grpc_mockserver_new" instead.

    @ingroup userver_testsuite_fixtures
    """
    server = grpc.aio.server()
    server.add_insecure_port(grpc_mockserver_endpoint)

    async with pytest_userver.grpc.MockserverSession(server=server, experimental=True) as mockserver:
        yield mockserver


@pytest.fixture
def grpc_mockserver_new(grpc_mockserver_session) -> pytest_userver.grpc.Mockserver:
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

    It supports the ability to raise mocked errors from the mockserver,
    which will cause the gRPC client to throw an exception.
    Types of supported errors:
    * @ref pytest_userver.grpc.NetworkError
    * @ref pytest_userver.grpc.TimeoutError

    @ingroup userver_testsuite_fixtures
    """
    try:
        yield pytest_userver.grpc.Mockserver(mockserver_session=grpc_mockserver_session, experimental=True)
    finally:
        grpc_mockserver_session.reset_mocks()


@pytest.fixture(scope='session')
def userver_config_grpc_mockserver(grpc_mockserver_endpoint):
    """
    Returns a function that adjusts the static config for testsuite.
    Finds `grpc-client-middleware-pipeline` in config_yaml and
    enables `grpc-client-middleware-testsuite`.

    @ingroup userver_testsuite_fixtures
    """

    def get_dict_field(parent: dict, field_name: str) -> dict:
        if parent.setdefault(field_name, {}) is None:
            parent[field_name] = {}

        return parent[field_name]

    def patch_config(config_yaml, _config_vars):
        components = config_yaml['components_manager']['components']
        if components.get('grpc-client-common', None) is not None:
            client_middlewares_pipeline = get_dict_field(components, 'grpc-client-middlewares-pipeline')
            middlewares = get_dict_field(client_middlewares_pipeline, 'middlewares')
            testsuite_middleware = get_dict_field(middlewares, 'grpc-client-middleware-testsuite')
            testsuite_middleware['enabled'] = True

    return patch_config


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


# @endcond
