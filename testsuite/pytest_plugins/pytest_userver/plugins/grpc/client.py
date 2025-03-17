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

USERVER_CONFIG_HOOKS = ['userver_config_grpc_endpoint']


@pytest.fixture(scope='session')
def grpc_service_port(service_config) -> Optional[int]:
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
    return components['grpc-server'].get('port', None)


@pytest.fixture(scope='session')
def grpc_service_port_fallback() -> int:
    """
    Returns the gRPC port that should be used in service runner mode in case
    no port is specified in the source config_yaml.

    @ingroup userver_testsuite_fixtures
    """
    return 11080


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
        f'unix:{grpc_server_unix_socket}' if grpc_server_unix_socket is not None else f'localhost:{grpc_service_port}'
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
    service_client,
    _testsuite_client_config: client.TestsuiteClientConfig,
) -> Callable[[grpc.aio.ClientCallDetails], Awaitable[None]]:
    """
    Returns the function that will be called in before each gRPC request,
    client-side.

    @ingroup userver_testsuite_fixtures
    """

    async def prepare(
        _client_call_details: grpc.aio.ClientCallDetails,
        /,
    ) -> None:
        if hasattr(service_client, 'update_server_state'):
            await service_client.update_server_state()

    return prepare


@pytest.fixture(scope='session')
async def grpc_session_channel(
    grpc_service_endpoint,
    _grpc_channel_interceptor,
):
    async with grpc.aio.insecure_channel(
        grpc_service_endpoint,
        interceptors=[_grpc_channel_interceptor],
    ) as channel:
        yield channel


@pytest.fixture
async def grpc_channel(
    service_client,  # For daemon setup and userver_client_cleanup
    grpc_service_endpoint,
    grpc_service_timeout,
    grpc_session_channel,
    _grpc_channel_interceptor,
    grpc_client_prepare,
):
    """
    Returns the gRPC channel configured by the parameters from the
    @ref pytest_userver.plugins.grpc.client.grpc_service_endpoint "grpc_service_endpoint" fixture.

    @ingroup userver_testsuite_fixtures
    """
    _grpc_channel_interceptor.prepare_func = grpc_client_prepare
    try:
        await asyncio.wait_for(
            grpc_session_channel.channel_ready(),
            timeout=grpc_service_timeout,
        )
    except asyncio.TimeoutError:
        raise RuntimeError(
            f'Failed to connect to remote gRPC server by address {grpc_service_endpoint}',
        )
    return grpc_session_channel


@pytest.fixture(scope='session')
def userver_config_grpc_endpoint(
    pytestconfig,
    grpc_service_port_fallback,
    substitute_config_vars,
):
    """
    Returns a function that adjusts the static config for testsuite.
    Ensures that in service runner mode, Unix socket is never used.

    @ingroup userver_testsuite_fixtures
    """

    def patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        grpc_server = components.get('grpc-server', None)
        if not grpc_server:
            return

        service_runner = pytestconfig.option.service_runner_mode
        if not service_runner:
            return

        grpc_server.pop('unix-socket-path', None)
        if 'port' not in substitute_config_vars(grpc_server, config_vars):
            grpc_server['port'] = grpc_service_port_fallback
        config_vars['grpc_server_port'] = grpc_service_port_fallback

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
        self.prepare_func: Optional[Callable[[grpc.aio.ClientCallDetails], Awaitable[None]]] = None

    async def intercept_unary_unary(
        self,
        continuation,
        client_call_details,
        request,
    ):
        await self.prepare_func(client_call_details)
        return await continuation(client_call_details, request)

    async def intercept_unary_stream(
        self,
        continuation,
        client_call_details,
        request,
    ):
        await self.prepare_func(client_call_details)
        return continuation(client_call_details, next(request))

    async def intercept_stream_unary(
        self,
        continuation,
        client_call_details,
        request_iterator,
    ):
        await self.prepare_func(client_call_details)
        return await continuation(client_call_details, request_iterator)

    async def intercept_stream_stream(
        self,
        continuation,
        client_call_details,
        request_iterator,
    ):
        await self.prepare_func(client_call_details)
        return continuation(client_call_details, request_iterator)


@pytest.fixture(scope='session')
def _grpc_channel_interceptor(daemon_scoped_mark) -> _GenericClientInterceptor:
    return _GenericClientInterceptor()
