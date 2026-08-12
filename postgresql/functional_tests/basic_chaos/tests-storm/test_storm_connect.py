import asyncio
import logging

import pytest
import pytest_userver.utils.sync as sync

logger = logging.getLogger(__name__)

_SELECT_URL = '/chaos/postgres?type=select'
_SELECT_SMALL_TIMEOUT_URL = '/chaos/postgres?type=select-small-timeout'
_MAX_POOL_SIZE = 3


async def _wait_for_broken_connection(recent_conn_error_fired):
    await sync.wait(
        recent_conn_error_fired.is_set,
        relax_period_seconds=0.1,
        total_wait_seconds=5.0,
        failure_msg='Timeout while waiting for postgresql broken connection',
    )


@pytest.mark.config(
    POSTGRES_CONNLIMIT_MODE_AUTO_ENABLED=False,
    POSTGRES_CONNECTION_POOL_SETTINGS={
        '__default__': {
            'max_pool_size': _MAX_POOL_SIZE,
            'min_pool_size': 1,
            'max_queue_size': 200,
            'connecting_limit': 1,
            'connecting_interval_ms': 500,
        }
    },
)
async def test_storm_connecting_rate_limit_throttles_new_attempts(
    service_client,
    gate,
    monitor_client,
    testpoint,
):
    response = await service_client.get(_SELECT_URL)
    assert response.status == 200

    recent_conn_error_fired = asyncio.Event()

    @testpoint('pg_recent_conn_error')
    async def pg_recent_conn_error_hook(_data):
        recent_conn_error_fired.set()

    try:
        async with monitor_client.metrics_diff(
            prefix='postgresql.connections',
            diff_gauge=True,
        ) as diff:
            async with service_client.capture_logs() as capture:
                await gate.to_server_close_on_data()
                await gate.to_client_close_on_data()

                for _ in range(_MAX_POOL_SIZE):
                    asyncio.create_task(service_client.get(_SELECT_SMALL_TIMEOUT_URL))
                await _wait_for_broken_connection(recent_conn_error_fired)

                responses = await asyncio.gather(
                    *[service_client.get(_SELECT_SMALL_TIMEOUT_URL) for _ in range(_MAX_POOL_SIZE * 4)],
                )
                assert all(r.status == 500 for r in responses)

        throttled_logs = [
            log
            for log in capture.select()
            if 'Connection rate limit exceeded, skipping new connection attempt' in log['text']
        ]
        assert throttled_logs
        assert diff.value_at('rate-limit-throttled') > 0
    finally:
        await gate.to_server_pass()
        await gate.to_client_pass()
        gate.start_accepting()
