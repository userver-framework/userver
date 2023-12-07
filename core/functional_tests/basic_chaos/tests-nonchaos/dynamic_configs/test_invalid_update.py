import re

import pytest

from testsuite.utils import http


async def test_ok(service_client, monitor_client):
    # Discard possible effects of the previous tests.
    # `fired_alerts` does not call `update_server_state` by default.
    await service_client.update_server_state()

    assert await monitor_client.fired_alerts() == []


CONFIG_PATTERN = (
    'Failed to parse dynamic config, go and fix it in tariff-editor: '
    'While parsing dynamic config values: '
    'Field \'HTTP_CLIENT_CONNECTION_POOL_SIZE\' is of a wrong '
    'type. Expected: uintValue, actual: stringValue '
    '\\([a-zA-Z0-9_:]*formats::json::TypeMismatchException\\)'
)


def validate_alerts(alerts):
    assert len(alerts) == 1
    assert alerts[0]['id'] == 'config_parse_error'
    assert re.match(CONFIG_PATTERN, alerts[0]['message'])


@pytest.fixture(name='break_config')
def _break_config(service_client, dynamic_config):
    async def do_break():
        dynamic_config.set(HTTP_CLIENT_CONNECTION_POOL_SIZE='invalid')
        with pytest.raises(http.HttpResponseError):
            await service_client.update_server_state()

    return do_break


@pytest.fixture(name='fix_config')
def _fix_config(service_client, dynamic_config):
    async def do_fix():
        dynamic_config.set(HTTP_CLIENT_CONNECTION_POOL_SIZE=1000)
        await service_client.update_server_state()

    return do_fix


async def test_invalid_config_alerts(
        service_client, monitor_client, break_config, fix_config,
):
    # Discard possible effects of the previous tests.
    # `fired_alerts` does not call `update_server_state` by default.
    await service_client.update_server_state()
    assert await monitor_client.fired_alerts() == []

    await break_config()
    alerts = await monitor_client.fired_alerts()
    validate_alerts(alerts)

    await fix_config()
    assert await monitor_client.fired_alerts() == []


@pytest.mark.now('2019-01-01T12:00:00+0000')
async def test_alert_active_after_1_year(
        service_client, monitor_client, mocked_time, break_config, fix_config,
):
    # Discard possible effects of the previous tests.
    # `fired_alerts` does not call `update_server_state` by default.
    await service_client.update_server_state()
    assert await monitor_client.fired_alerts() == []

    await break_config()

    alerts = await monitor_client.fired_alerts()
    validate_alerts(alerts)

    mocked_time.sleep(60 * 60 * 24 * 365)  # 1 year
    await service_client.tests_control(invalidate_caches=False)
    alerts = await monitor_client.fired_alerts()
    validate_alerts(alerts)

    await fix_config()
    assert await monitor_client.fired_alerts() == []


SUCCESS_GAUGE_METRIC = 'dynamic-config.was-last-parse-successful'
ERROR_RATE_METRIC = 'dynamic-config.parse-errors'


async def test_invalid_config_stats(
        service_client, monitor_client, break_config, fix_config,
):
    # Discard possible effects of the previous tests.
    # `monitor_client.metrics` does not call `update_server_state` by default.
    await service_client.update_server_state()

    metrics1 = await monitor_client.metrics(prefix='dynamic-config')
    await break_config()
    metrics2 = await monitor_client.metrics(prefix='dynamic-config')
    await fix_config()
    metrics3 = await monitor_client.metrics(prefix='dynamic-config')

    assert metrics1.value_at(path=SUCCESS_GAUGE_METRIC) == 1
    assert metrics2.value_at(path=SUCCESS_GAUGE_METRIC) == 0
    assert metrics3.value_at(path=SUCCESS_GAUGE_METRIC) == 1

    initial_errors = metrics1.value_at(path=ERROR_RATE_METRIC)
    assert metrics1.value_at(path=ERROR_RATE_METRIC) == initial_errors
    assert metrics2.value_at(path=ERROR_RATE_METRIC) == initial_errors + 1
    assert metrics3.value_at(path=ERROR_RATE_METRIC) == initial_errors + 1
