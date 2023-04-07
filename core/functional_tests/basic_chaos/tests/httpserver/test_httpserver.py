import asyncio
import enum
import logging
import typing

from aiohttp import client_exceptions as exceptions
import pytest

from pytest_userver import chaos

from testsuite.utils import http

HEADERS = {'Connection': 'keep-alive'}
DEFAULT_TIMEOUT = 5.0
DEFAULT_DATA = {'hello': 'world'}

DATA_PARTS_MAX_SIZE = 10

logger = logging.getLogger(__name__)


class ErrorType(enum.Enum):
    TIMEOUT = 1
    DISCONNECT = 2
    RESET_BY_PEER = 3
    BAD_REQUEST = 4
    UNKNOWN = -1


@pytest.fixture
def call(modified_service_client):
    async def _call(
            htype: str = 'common',
            data: typing.Optional[typing.Dict] = None,
            timeout: float = DEFAULT_TIMEOUT,
            tests_control: bool = False,
            **args,
    ) -> typing.Union[http.ClientResponse, ErrorType]:
        try:
            if not data:
                data = DEFAULT_DATA
            return await modified_service_client.get(
                '/chaos/httpserver',
                headers=HEADERS,
                timeout=timeout,
                params={'type': htype},
                data=data,
                testsuite_skip_prepare=not tests_control,
            )
        except asyncio.TimeoutError:
            return ErrorType.TIMEOUT
        except exceptions.ServerDisconnectedError:
            return ErrorType.DISCONNECT
        except exceptions.ClientOSError:
            return ErrorType.RESET_BY_PEER
        except exceptions.ClientResponseError:
            return ErrorType.BAD_REQUEST
        else:
            return ErrorType.UNKNOWN

    return _call


@pytest.fixture
def check_restore(gate, call):
    async def _check_restore():
        gate.to_server_pass()
        gate.to_client_pass()
        gate.start_accepting()

        response = await call()

        assert isinstance(response, http.ClientResponse)
        assert response.status == 200
        assert response.text == 'OK!'

    return _check_restore


async def chaos_stop_accepting(gate: chaos.TcpGate) -> None:
    await gate.stop_accepting()
    await gate.sockets_close()  # close keepalive connections
    assert gate.connections_count() == 0


async def test_ok(call, gate):
    response = await call()
    assert response.status == 200
    assert response.text == 'OK!'


async def test_stop_accepting(call, gate, check_restore):
    response = await call()
    assert response.status == 200
    assert gate.connections_count() >= 1

    await chaos_stop_accepting(gate)

    response = await call()
    assert isinstance(response, ErrorType)
    assert gate.connections_count() == 0

    gate.start_accepting()

    await check_restore()


async def test_close_during_request(call, gate, testpoint, check_restore):
    @testpoint('testpoint_request')
    async def _hook(data):
        if on:
            await gate.sockets_close()

    on = True
    response = await call(tests_control=True)
    assert response == ErrorType.DISCONNECT

    on = False
    response = await call()
    assert response.status == 200
    assert gate.connections_count() >= 1


async def test_close_on_data(call, gate, check_restore):
    response = await call()
    assert response.status == 200

    gate.to_server_close_on_data()

    assert gate.connections_count() >= 1
    for _ in range(gate.connections_count()):
        response = await call()
        assert response == ErrorType.RESET_BY_PEER

    await check_restore()


@pytest.mark.skip(reason='corrupted data can still be valid')
async def test_corrupted_request(call, gate, check_restore):
    gate.to_server_corrupt_data()

    response = await call()
    assert isinstance(response, http.ClientResponse)
    assert response.status == 400

    gate.to_server_pass()

    # Connection could be cached in testsuite client. Give it a few attempts
    # to restore
    for _ in range(15):
        resp = await call()
        if isinstance(resp, http.ClientResponse) and resp.status == 200:
            break

    await check_restore()


async def test_partial_request(call, gate, check_restore):
    success: bool = False
    fail: int = 0
    for bytes_count in range(1, 1000):
        gate.to_server_limit_bytes(bytes_count)
        response = await call(data={'test': 'body'})
        if response == ErrorType.DISCONNECT:
            fail = fail + 1
        elif isinstance(response, http.ClientResponse):
            success = True
            break
        else:
            logger.error(f'Got unexpected error {response}')
            assert False

    assert fail >= 250
    assert success
    assert isinstance(response, http.ClientResponse)
    assert response.status == 200

    await check_restore()


async def test_network_smaller_parts_sends(call, gate, check_restore):
    gate.to_server_smaller_parts(DATA_PARTS_MAX_SIZE)

    # With debug enabled in python send works a little bit longer
    response = await call(timeout=20.0)
    assert isinstance(response, http.ClientResponse)
    assert response.status == 200
