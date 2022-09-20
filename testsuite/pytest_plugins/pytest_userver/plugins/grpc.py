import asyncio

import grpc
import pytest

DEFAULT_TIMEOUT = 15.0


@pytest.fixture(scope='session')
def grpc_service_port(service_config_yaml):
    components = service_config_yaml['components_manager']['components']
    if 'grpc-server' not in components:
        raise RuntimeError('No grpc-server component')
    return components['grpc-server']['port']


@pytest.fixture(scope='session')
def grpc_service_endpoint(grpc_service_port):
    return f'localhost:{grpc_service_port}'


@pytest.fixture(scope='session')
def grpc_service_timeout(pytestconfig):
    return pytestconfig.option.service_timeout or DEFAULT_TIMEOUT


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


@pytest.fixture
def grpc_service_deps(service_client):
    pass
