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

    @snippet grpc/functional_tests/basic_chaos/tests-grpcclient/conftest.py grpc_mockserver_endpoint example
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
    @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver "grpc_mockserver" instead.

    @ingroup userver_testsuite_fixtures
    """
    server = grpc.aio.server()
    server.add_insecure_port(grpc_mockserver_endpoint)

    async with pytest_userver.grpc.MockserverSession(server=server, experimental=True) as mockserver:
        yield mockserver


@pytest.fixture
def grpc_mockserver(
    grpc_mockserver_session,
    asyncexc_append,
    _grpc_mockserver_ignore_errors,
) -> pytest_userver.grpc.Mockserver:
    """
    Returns the gRPC mocking server.
    In order for gRPC clients in your service to work, mock handlers need to be installed for them using this fixture.

    Example:

    @snippet samples/grpc_service/testsuite/test_grpc.py  Prepare modules
    @snippet samples/grpc_service/testsuite/test_grpc.py  grpc client test

    Alternatively, you can create a shorthand for mocking frequently-used services:

    @snippet grpc/functional_tests/metrics/tests/conftest.py  Prepare modules
    @snippet grpc/functional_tests/metrics/tests/conftest.py  Prepare server mock
    @snippet grpc/functional_tests/metrics/tests/test_metrics.py  grpc client test

    Mocks are only active within tests after their respective handler functions are created, not between tests.
    If the service needs the mock during startup, add the fixture that defines your mock to
    @ref pytest_userver.plugins.service.extra_client_deps "extra_client_deps".

    To return an error status instead of response, use `context` (see
    [ServicerContext](https://grpc.github.io/grpc/python/grpc_asyncio.html#grpc.aio.ServicerContext)
    docs):

    @snippet grpc/functional_tests/middleware_client/tests/test_error_status.py  Mocked error status

    To trigger special exceptions in the service's gRPC client, raise these mocked errors from the mock handler:

    * @ref pytest_userver.grpc._mocked_errors.TimeoutError "pytest_userver.grpc.TimeoutError"
    * @ref pytest_userver.grpc._mocked_errors.NetworkError "pytest_userver.grpc.NetworkError"

    @ingroup userver_testsuite_fixtures
    """
    with grpc_mockserver_session.asyncexc_append_scope(None if _grpc_mockserver_ignore_errors else asyncexc_append):
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


@pytest.fixture
def grpc_mockserver_new(grpc_mockserver) -> pytest_userver.grpc.Mockserver:
    """
    @deprecated Legacy alias for @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver "grpc_mockserver".
    """
    return grpc_mockserver


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


@pytest.fixture(scope='session')
def _grpc_mockserver_ignore_errors() -> bool:
    return False


# @endcond
