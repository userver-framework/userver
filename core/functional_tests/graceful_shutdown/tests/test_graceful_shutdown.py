import asyncio
from signal import SIGTERM

import pytest


@pytest.mark.uservice_oneshot
async def test_graceful_shutdown_timer_and_header(
    service_daemon_instance,
    service_client,
    monitor_client,
    dynamic_config,
):
    params = [
        (True, {'x-envoy-immediate-health-check-fail': ['true']}),
        (False, {'x-envoy-immediate-health-check-fail': ['true']}),
        (True, {'x-hdr1': ['true', 'aaa'], 'x-hdr2': ['1', 'bbb', 'yyyy']}),
        (False, {'x-hdr1': ['true', 'aaa'], 'x-hdr2': ['1', 'bbb', 'yyyy']}),
    ]

    response = await service_client.get('/ping')
    assert response.status == 200

    for headers_enabled, headers in params:
        dynamic_config.set_values({'GRACEFUL_SHUTDOWN_HEADERS': {'enabled': headers_enabled, 'headers': headers}})
        await service_client.update_server_state()

        response = await monitor_client.get('/log-level/')
        assert response.status == 200
        for header, _ in headers.items():
            assert response.headers.get(header) is None

    service_daemon_instance.process.send_signal(SIGTERM)

    status = 200
    while status == 200:
        await asyncio.sleep(0.1)
        response = await service_client.get('/ping')
        status = response.status
    assert status == 500

    for headers_enabled, headers in params:
        dynamic_config.set_values({'GRACEFUL_SHUTDOWN_HEADERS': {'enabled': headers_enabled, 'headers': headers}})
        await service_client.update_server_state()

        response = await monitor_client.get('/log-level/')
        assert response.status == 200
        if headers_enabled:
            for header, values in headers.items():
                assert response.headers.get(header) == ', '.join(values)
        else:
            for header, _ in headers.items():
                assert response.headers.get(header) is None

    await asyncio.sleep(1)
    # Check that the service is still alive.
    response = await service_client.get('/ping')
    assert response.status == 500

    # After a couple more seconds, the service will start shutting down.
    service_daemon_instance.process.wait()
