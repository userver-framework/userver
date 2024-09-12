"""
Make gRPC requests to the service.

@sa @ref scripts/docs/en/userver/tutorial/grpc_service.md
"""

# pylint: disable=no-member
# pylint: disable=redefined-outer-name
import asyncio
from typing import Awaitable
from typing import Callable
from typing import Optional

import grpc
import pytest

from pytest_userver import client

DEFAULT_TIMEOUT = 15.0

USERVER_CONFIG_HOOKS = ['userver_config_grpc_mockserver']


@pytest.fixture(scope='session')
def grpc_service_port(service_config) -> int:
    """
    Returns the gRPC listener port number of the service that is set in the
    static configuration file.

    Override this fixture to change the way the gRPC listener port number
    is retrieved by the testsuite for tests.

    @ingroup userver_testsuite_fixtures
    """
    components = service_config['components_manager']['components']
    if 'grpc-server' not in components:
        raise RuntimeError('No grpc-server component')
    return int(components['grpc-server']['port'])


@pytest.fixture(scope='session')
def grpc_service_endpoint(service_config, grpc_service_port) -> str:
    """
    Returns the gRPC endpoint of the service.

    Override this fixture to change the way the gRPC endpoint
    is retrieved by the testsuite for tests.

    @ingroup userver_testsuite_fixtures
    """
    components = service_config['components_manager']['components']
    if 'grpc-server' not in components:
        raise RuntimeError('No grpc-server component')
    grpc_server_unix_socket = components['grpc-server'].get('unix-socket-path')
    return (
        f'unix:{grpc_server_unix_socket}'
        if grpc_server_unix_socket is not None
        else f'localhost:{grpc_service_port}'
    )


@pytest.fixture(scope='session')
def grpc_service_timeout(pytestconfig) -> float:
    """
    Returns the gRPC timeout for the service that is set by the command
    line option `--service-timeout`.

    Override this fixture to change the way the gRPC timeout
    is set.

    @ingroup userver_testsuite_fixtures
    """
    return float(pytestconfig.option.service_timeout) or DEFAULT_TIMEOUT


@pytest.fixture
def grpc_client_prepare(
    service_client, _testsuite_client_config: client.TestsuiteClientConfig,
) -> Callable[[grpc.aio.ClientCallDetails], Awaitable[None]]:
    """
    Returns the function that will be called in before each gRPC request,
    client-side.

    @ingroup userver_testsuite_fixtures
    """

    async def prepare(
        _client_call_details: grpc.aio.ClientCallDetails, /,
    ) -> None:
        if isinstance(service_client, client.AiohttpClient):
            await service_client.update_server_state()

    return prepare


@pytest.fixture(scope='session')
async def _grpc_session_channel(
    grpc_service_endpoint, _grpc_channel_interceptor,
):
    async with grpc.aio.insecure_channel(
        grpc_service_endpoint, interceptors=[_grpc_channel_interceptor],
    ) as channel:
        yield channel


@pytest.fixture
async def grpc_channel(
    grpc_service_endpoint,
    grpc_service_deps,
    grpc_service_timeout,
    _grpc_session_channel,
    _grpc_channel_interceptor,
    grpc_client_prepare,
):
    """
    Returns the gRPC channel configured by the parameters from the
    @ref plugins.grpc.grpc_service_endpoint "grpc_service_endpoint" fixture.

    @ingroup userver_testsuite_fixtures
    """
    _grpc_channel_interceptor.prepare_func = grpc_client_prepare
    try:
        await asyncio.wait_for(
            _grpc_session_channel.channel_ready(), timeout=grpc_service_timeout,
        )
    except asyncio.TimeoutError:
        raise RuntimeError(
            f'Failed to connect to remote gRPC server by '
            f'address {grpc_service_endpoint}',
        )
    return _grpc_session_channel


@pytest.fixture
def grpc_service_deps(service_client):
    """
    gRPC service dependencies hook. Feel free to override it.

    @ingroup userver_testsuite_fixtures
    """


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


# Taken from
# https://github.com/grpc/grpc/blob/master/examples/python/interceptors/headers/generic_client_interceptor.py
class _GenericClientInterceptor(
    grpc.aio.UnaryUnaryClientInterceptor,
    grpc.aio.UnaryStreamClientInterceptor,
    grpc.aio.StreamUnaryClientInterceptor,
    grpc.aio.StreamStreamClientInterceptor,
):
    def __init__(self):
        self.prepare_func: Optional[
            Callable[[grpc.aio.ClientCallDetails], Awaitable[None]]
        ] = None

    async def intercept_unary_unary(
        self, continuation, client_call_details, request,
    ):
        await self.prepare_func(client_call_details)
        return await continuation(client_call_details, request)

    async def intercept_unary_stream(
        self, continuation, client_call_details, request,
    ):
        await self.prepare_func(client_call_details)
        return await continuation(client_call_details, next(request))

    async def intercept_stream_unary(
        self, continuation, client_call_details, request_iterator,
    ):
        await self.prepare_func(client_call_details)
        return await continuation(client_call_details, request_iterator)

    async def intercept_stream_stream(
        self, continuation, client_call_details, request_iterator,
    ):
        await self.prepare_func(client_call_details)
        return await continuation(client_call_details, request_iterator)


@pytest.fixture(scope='session')
def _grpc_channel_interceptor() -> _GenericClientInterceptor:
    return _GenericClientInterceptor()
