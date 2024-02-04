import re

import pytest

from testsuite.utils import http


@pytest.fixture(name='assert_dynconf_update_fails')
async def _assert_dynconf_update_fails(service_client):
    async with service_client.capture_logs(
            testsuite_skip_prepare=True,
    ) as capture:
        # Update failure is expected, because all caches are required to update
        # successfully, but dynamic-config-client-updater update will fail.
        with pytest.raises(http.HttpResponseError):
            await service_client.update_server_state()
    expected_text_regex = re.compile(
        r'code = 500 \([\w:]*clients::http::HttpServerException\)',
    )
    assert any(
        expected_text_regex.search(record['text'])
        for record in capture.select()
    )


async def test_ping(service_client, assert_dynconf_update_fails):
    response = await service_client.get('ping', testsuite_skip_prepare=True)
    assert response.status_code == 200
    assert response.content == b''


_INF_DURATION_MS = 100_000_000


async def test_all_dynconf_updates_failed(
        monitor_client, assert_dynconf_update_fails,
):
    snapshot = await monitor_client.metrics(
        prefix='cache', labels={'cache_name': 'dynamic-config-client-updater'},
    )

    ms_since_last_update_attempt = snapshot.value_at(
        'cache.any.time.time-from-last-update-start-ms',
    )
    ms_since_last_update_success = snapshot.value_at(
        'cache.any.time.time-from-last-successful-start-ms',
    )
    assert ms_since_last_update_attempt < _INF_DURATION_MS
    assert ms_since_last_update_success > _INF_DURATION_MS

    attempts = snapshot.value_at('cache.any.update.attempts_count')
    failures = snapshot.value_at('cache.any.update.failures_count')
    assert attempts > 0
    assert attempts == failures
