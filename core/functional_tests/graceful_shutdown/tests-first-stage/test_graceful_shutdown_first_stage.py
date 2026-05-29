import asyncio
from signal import SIGTERM

import pytest
import pytest_userver.utils.sync as sync


@pytest.mark.uservice_oneshot
async def test_graceful_shutdown_first_stage(
    service_daemon_instance,
    service_client,
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

        response = await service_client.get('/test')
        assert response.status == 200
        for header, _ in headers.items():
            assert response.headers.get(header) is None

    service_daemon_instance.process.send_signal(SIGTERM)

    async def is_failing():
        response = await service_client.get('/ping')
        return response.status == 500

    await sync.wait(is_failing, relax_period_seconds=0.1)

    for headers_enabled, headers in params:
        dynamic_config.set_values({'GRACEFUL_SHUTDOWN_HEADERS': {'enabled': headers_enabled, 'headers': headers}})
        await service_client.update_server_state()

        response = await service_client.get('/test')
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
