"""
Make gRPC requests to the service.

@sa @ref scripts/docs/en/userver/tutorial/grpc_service.md
"""

# pylint: disable=no-member
import asyncio

import grpc
import pytest

DEFAULT_TIMEOUT = 15.0

USERVER_CONFIG_HOOKS = ['prepare_config_vars']


@pytest.fixture(name='grpc_service_port', scope='session')
def _grpc_service_port(service_config_yaml) -> int:
    """
    Returns the gRPC listener port number of the service that is set in the
    static configuration file.

    Override this fixture to change the way the gRPC listener port number
    is retrieved by the testsuite for tests.

    @ingroup userver_testsuite_fixtures
    """
    components = service_config_yaml['components_manager']['components']
    if 'grpc-server' not in components:
        raise RuntimeError('No grpc-server component')
    return components['grpc-server']['port']


@pytest.fixture(name='grpc_service_endpoint', scope='session')
def _grpc_service_endpoint(grpc_service_port) -> str:
    """
    Returns the gRPC endpoint of the service.

    Override this fixture to change the way the gRPC endpoint
    is retrieved by the testsuite for tests.

    @ingroup userver_testsuite_fixtures
    """
    return f'localhost:{grpc_service_port}'


@pytest.fixture(name='grpc_service_timeout', scope='session')
def _grpc_service_timeout(pytestconfig) -> float:
    """
    Returns the gRPC timeout for the service that is set by the command
    line option `--service-timeout`.

    Override this fixture to change the way the gRPC timeout
    is set.

    @ingroup userver_testsuite_fixtures
    """
    return float(pytestconfig.option.service_timeout) or DEFAULT_TIMEOUT


@pytest.fixture(scope='session')
async def _grpc_session_channel(grpc_service_endpoint):
    async with grpc.aio.insecure_channel(grpc_service_endpoint) as channel:
        yield channel


@pytest.fixture
async def grpc_channel(
        grpc_service_endpoint,
        grpc_service_deps,
        grpc_service_timeout,
        _grpc_session_channel,
):
    """
    Returns the gRPC channel configured by the parameters from the
    @ref plugins.grpc.grpc_service_endpoint "grpc_service_endpoint" fixture.

    @ingroup userver_testsuite_fixtures
    """
    try:
        await asyncio.wait_for(
            _grpc_session_channel.channel_ready(),
            timeout=grpc_service_timeout,
        )
    except asyncio.TimeoutError:
        raise RuntimeError(
            f'Failed to connect to remote gRPC server by '
            f'address {grpc_service_endpoint}',
        )
    return _grpc_session_channel


@pytest.fixture(name='grpc_service_deps')
def _grpc_service_deps(service_client):
    """
    gRPC service dependencies hook. Feel free to override it.

    @ingroup userver_testsuite_fixtures
    """


@pytest.fixture(scope='session')
def prepare_config_vars(grpc_mockserver_endpoint):
    def patch_config(config, config_vars):
        for name in config_vars:
            if config_vars[name] == '$grpc_mockserver':
                config_vars[name] = grpc_mockserver_endpoint

    return patch_config
