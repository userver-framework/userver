import asyncio
import typing

import pytest

from testsuite.utils import http

HEADERS = {'Connection': 'keep-alive'}
DEFAULT_TIMEOUT = 5.0
DEFAULT_DATA = {'hello': 'world'}

DP_TIMEOUT_MS = 'X-YaTaxi-Client-TimeoutMs'
DP_DEADLINE_EXPIRED = 'X-YaTaxi-Deadline-Expired'


@pytest.fixture(name='call')
def _call(service_client):
    async def _call(
            htype: str = 'common',
            data: typing.Any = None,
            timeout: float = DEFAULT_TIMEOUT,
            headers: typing.Optional[typing.Dict[str, str]] = None,
    ) -> http.ClientResponse:
        if not data:
            data = DEFAULT_DATA
        if headers is None:
            headers = HEADERS
        return await service_client.get(
            '/chaos/httpserver',
            headers=headers,
            timeout=timeout,
            params={'type': htype},
            data=data,
        )

    return _call


def _check_deadline_propagation_response(response):
    assert isinstance(response, http.ClientResponse)
    assert DP_DEADLINE_EXPIRED in response.headers
    # Check that the status code is taken from deadline_expired_status_code.
    assert response.status == 504
    # Check that the error format is customizable
    # by GetFormattedExternalErrorBody.
    assert 'application/json' in response.headers['content-type']
    assert response.json()['message'] == 'Deadline expired'


@pytest.fixture(name='handler_metrics')
async def _handler_metrics(monitor_client):
    return monitor_client.metrics_diff(
        prefix='http.handler.total', diff_gauge=True,
    )


async def test_deadline_expired(
        call, testpoint, service_client, handler_metrics,
):
    @testpoint('testpoint_request')
    async def test(_data):
        pass

    # Avoid 'prepare' being accounted in the client metrics diff.
    await service_client.update_server_state()

    async with handler_metrics:
        response = await call(htype='sleep', headers={DP_TIMEOUT_MS: '150'})
        _check_deadline_propagation_response(response)
        assert (
            test.times_called == 1
        ), 'Control flow SHOULD enter the handler body'

    assert handler_metrics.value_at('rps') == 1
    assert (
        handler_metrics.value_at(
            'reply-codes', {'http_code': '504', 'version': '2'},
        )
        == 1
    )
    assert handler_metrics.value_at('deadline-received') == 1
    assert handler_metrics.value_at('cancelled-by-deadline') == 1


@pytest.mark.config(USERVER_DEADLINE_PROPAGATION_ENABLED=False)
async def test_deadline_propagation_disabled_dynamically(call):
    response = await call(htype='sleep', headers={DP_TIMEOUT_MS: '10'})
    assert isinstance(response, http.ClientResponse)
    assert response.status == 200


async def test_cancellable(service_client, testpoint):
    @testpoint('testpoint_cancel')
    def cancel_testpoint(data):
        pass

    less_than_handler_sleep_time = 1.0
    with pytest.raises(asyncio.TimeoutError):
        await service_client.get(
            '/chaos/httpserver',
            params={'type': 'cancel'},
            timeout=less_than_handler_sleep_time,
        )
    await cancel_testpoint.wait_call()
