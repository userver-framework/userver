import asyncio
import asyncio.streams as streams
import asyncio.trsock as trsock
import json
from typing import Any
from typing import AsyncGenerator
from typing import Callable
from typing import Dict

import grpc
import http2
import pytest

import testsuite.utils.net as net_utils

import utils

pytest_plugins = [
    'pytest_userver.plugins.core',
]

USERVER_CONFIG_HOOKS = ['userver_config_http2_server_endpoint']


@pytest.fixture(scope='session')
def retryable_status_codes(_retry_policy) -> list[grpc.StatusCode]:
    return [utils.status_from_str(status) for status in _retry_policy['retryableStatusCodes']]


@pytest.fixture(scope='session')
def max_attempts(_retry_policy) -> int:
    return _retry_policy['maxAttempts']


@pytest.fixture(scope='session')
def _retry_policy(_default_service_config) -> Dict[str, Any]:
    return _default_service_config['methodConfig'][0]['retryPolicy']


@pytest.fixture(scope='session')
def _default_service_config(service_config) -> Dict[str, Any]:
    return json.loads(
        service_config['components_manager']['components']['grpc-client-factory']['default-service-config']
    )


@pytest.fixture(scope='session', autouse=True)
async def grpc_server() -> AsyncGenerator[http2.GrpcServer, None]:
    server = http2.GrpcServer()

    def _protocol_factory():
        _loop = asyncio.get_running_loop()
        _reader = streams.StreamReader(loop=_loop)
        return streams.StreamReaderProtocol(_reader, server, loop=_loop)

    async with net_utils.create_tcp_server(_protocol_factory) as s:
        socket: trsock.TransportSocket = s.sockets[0]
        endpoint = ':'.join(map(str, socket.getsockname()))

        server.endpoint = endpoint

        yield server


@pytest.fixture(scope='session')
def userver_config_http2_server_endpoint(
    grpc_server: http2.GrpcServer,
) -> Callable[[Any, Any], None]:
    def _patch_config(config_yaml, _config_vars):
        components = config_yaml['components_manager']['components']

        components['client-runner']['server-endpoint'] = grpc_server.endpoint

    return _patch_config
