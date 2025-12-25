"""
Make gRPC requests to the service.

@sa @ref scripts/docs/en/userver/tutorial/grpc_service.md
"""

# pylint: disable=redefined-outer-name
import asyncio
from collections.abc import AsyncIterator
from collections.abc import Awaitable
from collections.abc import Callable
from collections.abc import Generator
from collections.abc import Sequence
import itertools
import pathlib
import tempfile
from typing import Any
from typing import TypeAlias

import grpc
import grpc.aio
import pytest

import pytest_userver.client
from . import _hookspec

DEFAULT_TIMEOUT = 15.0
USERVER_CONFIG_HOOKS = ['userver_config_grpc_endpoint']

_AsyncExcCheck: TypeAlias = Callable[[], None]


@pytest.fixture(scope='session')
def grpc_service_port_fallback() -> int:
    """
    Returns the gRPC port that should be used in service runner mode in case
    no port is specified in the source config_yaml.

    @ingroup userver_testsuite_fixtures
    """
    return 11080


@pytest.fixture(scope='session')
def grpc_service_endpoint(service_config) -> str:
    """
    Returns the gRPC endpoint of the service.
    Used by @ref pytest_userver.plugins.grpc.client.grpc_channel "grpc_channel" fixture.

    By default, gets the actual gRPC endpoint from `service_config`.
    Override this fixture if you add gRPC server listening ports in a custom way.

    @ingroup userver_testsuite_fixtures
    """
    components = service_config['components_manager']['components']
    if 'grpc-server' not in components:
        raise RuntimeError('No grpc-server component')
    grpc_server = components['grpc-server']

    if unix_socket_path := grpc_server.get('unix-socket-path'):
        return f'unix:{unix_socket_path}'
    if port := grpc_server.get('port'):
        return f'localhost:{port}'
    raise RuntimeError("No 'port' or 'unix-socket-path' found in 'grpc-server' component config")


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


@pytest.fixture(scope='session')
async def grpc_session_channel(grpc_service_endpoint):
    async with grpc.aio.insecure_channel(grpc_service_endpoint) as channel:
        yield channel


@pytest.fixture
async def grpc_channel(
    service_client,  # For daemon setup and userver_client_cleanup
    grpc_service_endpoint,
    grpc_service_timeout,
    grpc_session_channel,
    request,
):
    """
    Returns the gRPC channel configured by the parameters from the
    @ref pytest_userver.plugins.grpc.client.grpc_service_endpoint "grpc_service_endpoint" fixture.

    You can add interceptors to the channel by implementing the
    @ref pytest_userver.plugins.grpc._hookspec.pytest_grpc_client_interceptors "pytest_grpc_client_interceptors"
    hook in your pytest plugin or initial (root) conftest.

    @ingroup userver_testsuite_fixtures
    """
    interceptors = request.config.hook.pytest_grpc_client_interceptors(request=request)
    interceptors_list = list(itertools.chain.from_iterable(interceptors))
    # Sanity check: we have at least one "builtin" interceptor.
    assert len(interceptors_list) != 0

    try:
        await asyncio.wait_for(
            grpc_session_channel.channel_ready(),
            timeout=grpc_service_timeout,
        )
    except asyncio.TimeoutError:
        raise RuntimeError(
            f'Failed to connect to remote gRPC server by address {grpc_service_endpoint}',
        )

    _set_client_interceptors(grpc_session_channel, interceptors_list)
    try:
        yield grpc_session_channel
    finally:
        _set_client_interceptors(grpc_session_channel, [])


@pytest.fixture(scope='session')
def grpc_socket_path() -> Generator[pathlib.Path, None, None]:
    """
    Path for the UNIX socket over which testsuite will talk to the gRPC service, if it chooses to use a UNIX socket.

    @see pytest_userver.plugins.grpc.client.userver_config_grpc_endpoint "userver_config_grpc_endpoint"

    @ingroup userver_testsuite_fixtures
    """
    # Path must be as short as possible due to 108 character limitation.
    # 'service_tempdir', for example, is typically too long.
    with tempfile.TemporaryDirectory(prefix='userver-grpc-socket-') as name:
        yield pathlib.Path(name) / 'grpc.sock'


@pytest.fixture(scope='session')
def userver_config_grpc_endpoint(
    pytestconfig,
    grpc_service_port_fallback,
    substitute_config_vars,
    request,
    choose_free_port,
):
    """
    Returns a function that adjusts the static config for testsuite.

    * if the original service config specifies `grpc-server.port`, and that port is taken,
      then adjusts it to a free port;
    * if the original service config specifies `grpc-server.unix-socket-path`,
      then adjusts it to a tmp path
      (see @ref pytest_userver.plugins.grpc.client.grpc_socket_path "grpc_socket_path");
    * in service runner mode, uses the original grpc port from config or
      @ref pytest_userver.plugins.grpc.client.grpc_service_port_fallback "grpc_service_port_fallback".

    Override this fixture to change the way `grpc-server` endpoint config is patched for tests.

    @ingroup userver_testsuite_fixtures
    """

    def patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        grpc_server = components.get('grpc-server', None)
        if not grpc_server:
            return

        original_grpc_server = substitute_config_vars(grpc_server, config_vars)

        if pytestconfig.option.service_runner_mode:
            grpc_server.pop('unix-socket-path', None)
            if 'port' not in original_grpc_server:
                grpc_server['port'] = grpc_service_port_fallback
            config_vars['grpc_server_port'] = grpc_service_port_fallback
        elif 'unix-socket-path' in original_grpc_server:
            grpc_server.pop('port', None)
            grpc_socket_path = request.getfixturevalue('grpc_socket_path')
            grpc_server['unix-socket-path'] = str(grpc_socket_path)
        else:
            grpc_server.pop('unix-socket-path', None)
            grpc_server['port'] = choose_free_port(original_grpc_server.get('port', None))

    return patch_config


# @cond


def pytest_addhooks(pluginmanager: pytest.PytestPluginManager):
    pluginmanager.add_hookspecs(_hookspec)


class _UpdateServerStateInterceptor(
    grpc.aio.UnaryUnaryClientInterceptor,
    grpc.aio.UnaryStreamClientInterceptor,
    grpc.aio.StreamUnaryClientInterceptor,
    grpc.aio.StreamStreamClientInterceptor,
):
    def __init__(self, service_client: pytest_userver.client.Client, asyncexc_check: _AsyncExcCheck):
        self._service_client = service_client
        self._asyncexc_check = asyncexc_check

    async def _before_call_hook(self) -> None:
        self._asyncexc_check()
        if hasattr(self._service_client, 'update_server_state'):
            await self._service_client.update_server_state()

    async def intercept_unary_unary(
        self,
        continuation: Callable[[grpc.aio.ClientCallDetails, Any], Awaitable[grpc.aio.UnaryUnaryCall]],
        client_call_details: grpc.aio.ClientCallDetails,
        request: Any,
    ) -> grpc.aio.UnaryUnaryCall:
        await self._before_call_hook()
        try:
            return await continuation(client_call_details, request)
        finally:
            self._asyncexc_check()

    # Note: full type of this function is Callable[[...], Awaitable[AsyncIterator[Any]]]
    async def intercept_unary_stream(
        self,
        continuation: Callable[[grpc.aio.ClientCallDetails, Any], grpc.aio.UnaryStreamCall],
        client_call_details: grpc.aio.ClientCallDetails,
        request: Any,
    ) -> AsyncIterator[Any]:
        await self._before_call_hook()
        call = await continuation(client_call_details, request)

        async def response_stream() -> AsyncIterator[Any]:
            try:
                async for response in call:
                    yield response
            finally:
                self._asyncexc_check()

        return response_stream()

    async def intercept_stream_unary(
        self,
        continuation: Callable[[grpc.aio.ClientCallDetails, AsyncIterator[Any]], Awaitable[grpc.aio.StreamUnaryCall]],
        client_call_details: grpc.aio.ClientCallDetails,
        request_iterator: AsyncIterator[Any],
    ) -> grpc.aio.StreamUnaryCall:
        await self._before_call_hook()
        try:
            return await continuation(client_call_details, request_iterator)
        finally:
            self._asyncexc_check()

    # Note: full type of this function is Callable[[...], Awaitable[AsyncIterator[Any]]]
    async def intercept_stream_stream(
        self,
        continuation: Callable[[grpc.aio.ClientCallDetails, AsyncIterator[Any]], grpc.aio.StreamStreamCall],
        client_call_details: grpc.aio.ClientCallDetails,
        request_iterator: AsyncIterator[Any],
    ) -> AsyncIterator[Any]:
        self._asyncexc_check()
        await self._before_call_hook()
        call = await continuation(client_call_details, request_iterator)

        async def response_stream() -> AsyncIterator[Any]:
            try:
                async for response in call:
                    yield response
            finally:
                self._asyncexc_check()

        return response_stream()


def pytest_grpc_client_interceptors(request: pytest.FixtureRequest) -> Sequence[grpc.aio.ClientInterceptor]:
    return [
        _UpdateServerStateInterceptor(
            request.getfixturevalue('service_client'),
            request.getfixturevalue('asyncexc_check'),
        ),
    ]


def _filter_interceptors(
    interceptors: Sequence[grpc.aio.ClientInterceptor], desired_type: type[grpc.aio.ClientInterceptor]
) -> list[grpc.aio.ClientInterceptor]:
    return [interceptor for interceptor in interceptors if isinstance(interceptor, desired_type)]


def _set_client_interceptors(channel: grpc.aio.Channel, interceptors: Sequence[grpc.aio.ClientInterceptor]) -> None:
    """
    Allows to set interceptors dynamically while reusing the same underlying channel,
    which is something grpc-io currently doesn't support.

    Also fixes the bug: multi-inheritance interceptors are only registered for first matching type
    https://github.com/grpc/grpc/issues/31442
    """
    channel._unary_unary_interceptors = _filter_interceptors(interceptors, grpc.aio.UnaryUnaryClientInterceptor)
    channel._unary_stream_interceptors = _filter_interceptors(interceptors, grpc.aio.UnaryStreamClientInterceptor)
    channel._stream_unary_interceptors = _filter_interceptors(interceptors, grpc.aio.StreamUnaryClientInterceptor)
    channel._stream_stream_interceptors = _filter_interceptors(interceptors, grpc.aio.StreamStreamClientInterceptor)


# @endcond
