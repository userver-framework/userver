# pylint: disable=broad-except
import asyncio
import enum
import gzip
import logging
import typing

from aiohttp import client_exceptions as exceptions
import pytest

from pytest_userver import chaos

from testsuite.utils import http

HEADERS = {'Connection': 'keep-alive'}
DEFAULT_TIMEOUT = 5.0
INCREASED_TIMEOUT = 20.0
DEFAULT_DATA = {'hello': 'world'}

DATA_PARTS_MAX_SIZE = 10
BYTES_PER_SECOND_LIMIT = 10

logger = logging.getLogger(__name__)


class ErrorType(enum.Enum):
    TIMEOUT = 1
    DISCONNECT = 2
    RESET_BY_PEER = 3
    BAD_REQUEST = 4
    UNKNOWN = -1


@pytest.fixture
def call(modified_service_client, gate):
    async def _call(
            htype: str = 'common',
            data: typing.Any = None,
            timeout: float = DEFAULT_TIMEOUT,
            testsuite_skip_prepare: bool = False,
            headers: typing.Optional[typing.Dict[str, str]] = None,
    ) -> typing.Union[http.ClientResponse, ErrorType]:
        try:
            if not data:
                data = DEFAULT_DATA
            if headers is None:
                headers = HEADERS
            return await modified_service_client.get(
                '/chaos/httpserver',
                headers=headers,
                timeout=timeout,
                params={'type': htype},
                data=data,
                testsuite_skip_prepare=testsuite_skip_prepare,
            )
        except asyncio.TimeoutError:
            return ErrorType.TIMEOUT
        except exceptions.ServerDisconnectedError:
            return ErrorType.DISCONNECT
        except exceptions.ClientOSError:
            return ErrorType.RESET_BY_PEER
        except exceptions.ClientResponseError:
            return ErrorType.BAD_REQUEST
        except Exception as ex:
            logger.error(f'Unknown exception {ex}')
            return ErrorType.UNKNOWN

    return _call


@pytest.fixture
def check_restore(gate, call):
    async def _check_restore():
        gate.to_server_pass()
        gate.to_client_pass()
        gate.start_accepting()

        response = await call(testsuite_skip_prepare=True)

        assert isinstance(response, http.ClientResponse)
        assert response.status == 200
        assert response.text == 'OK!'

    return _check_restore


async def chaos_stop_accepting(gate: chaos.TcpGate) -> None:
    await gate.stop_accepting()
    await gate.sockets_close()  # close keepalive connections
    assert gate.connections_count() == 0


async def test_ok(call):
    response = await call(testsuite_skip_prepare=True)
    assert response.status == 200
    assert response.text == 'OK!'


async def test_ok_compressed(call):
    response = await call(
        headers={'content-encoding': 'gzip'},
        data=gzip.compress('abcd'.encode()),
        testsuite_skip_prepare=True,
    )
    assert response.status == 200
    assert response.text == 'OK!'


async def test_stop_accepting(call, gate, check_restore):
    response = await call(testsuite_skip_prepare=True)
    assert response.status == 200
    assert gate.connections_count() >= 1

    await chaos_stop_accepting(gate)

    response = await call(testsuite_skip_prepare=True)
    assert isinstance(response, ErrorType)
    assert gate.connections_count() == 0

    gate.start_accepting()

    await check_restore()


async def test_close_during_request(call, gate, testpoint):
    @testpoint('testpoint_request')
    async def _hook(_data):
        if on:
            await gate.sockets_close()

    on = True
    response = await call()
    assert response == ErrorType.DISCONNECT

    on = False
    response = await call()
    assert response.status == 200
    assert gate.connections_count() >= 1


async def test_close_on_data(call, gate, check_restore):
    response = await call(testsuite_skip_prepare=True)
    assert response.status == 200

    gate.to_server_close_on_data()

    assert gate.connections_count() >= 1
    for _ in range(gate.connections_count()):
        response = await call(testsuite_skip_prepare=True)
        assert response == ErrorType.RESET_BY_PEER

    await check_restore()


@pytest.mark.skip(reason='corrupted data can still be valid')
async def test_corrupted_request(call, gate, check_restore):
    gate.to_server_corrupt_data()

    response = await call(testsuite_skip_prepare=True)
    assert isinstance(response, http.ClientResponse)
    assert response.status == 400

    gate.to_server_pass()

    # Connection could be cached in testsuite client. Give it a few attempts
    # to restore
    for _ in range(15):
        resp = await call(testsuite_skip_prepare=True)
        if isinstance(resp, http.ClientResponse) and resp.status == 200:
            break

    await check_restore()


async def test_partial_request(call, gate, check_restore):
    success: bool = False
    fail: int = 0
    for bytes_count in range(1, 1000):
        gate.to_server_limit_bytes(bytes_count)
        response = await call(
            data={'test': 'body'}, testsuite_skip_prepare=True,
        )
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


async def test_network_smaller_parts_sends(call, gate):
    gate.to_server_smaller_parts(DATA_PARTS_MAX_SIZE)

    # With debug enabled in python send works a little bit longer
    response = await call(
        timeout=INCREASED_TIMEOUT, testsuite_skip_prepare=True,
    )
    assert isinstance(response, http.ClientResponse)
    assert response.status == 200


def _check_deadline_propagation_response(response):
    assert isinstance(response, http.ClientResponse)
    assert response.status == 504
    # Check that the error format is customizable
    # by GetFormattedExternalErrorBody.
    assert 'application/json' in response.headers['content-type']
    assert response.json()['message'] == 'Deadline expired'


async def test_deadline_immediately_expired(call, gate, testpoint):
    testpoint_data = []

    @testpoint('testpoint_request')
    async def _hook(data):
        testpoint_data.append(data)

    gate.to_server_smaller_parts(DATA_PARTS_MAX_SIZE, sleep_per_packet=0.03)

    response = await call(
        headers={'X-YaTaxi-Client-TimeoutMs': '20'}, timeout=INCREASED_TIMEOUT,
    )
    _check_deadline_propagation_response(response)
    assert not testpoint_data, 'Control flow should NOT enter the handler body'


async def test_deadline_expired(call, testpoint, monitor_client):
    testpoint_data = []

    @testpoint('testpoint_request')
    async def _hook(data):
        testpoint_data.append(data)

    # TODO(TAXICOMMON-6876) make service_client.reset_metric()
    #  work for RegisterWriter metrics
    cancelled_metric_before = await monitor_client.single_metric(
        path='http.handler.total.cancelled-by-deadline',
    )

    response = await call(
        htype='sleep',
        headers={'X-YaTaxi-Client-TimeoutMs': '150'},
        timeout=INCREASED_TIMEOUT,
    )
    _check_deadline_propagation_response(response)
    assert testpoint_data, 'Control flow SHOULD enter the handler body'

    cancelled_metric_after = await monitor_client.single_metric(
        path='http.handler.total.cancelled-by-deadline',
    )
    assert cancelled_metric_after.value - cancelled_metric_before.value == 1
