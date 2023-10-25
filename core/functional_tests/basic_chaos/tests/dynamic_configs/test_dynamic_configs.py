import datetime
import re

import pytest

from testsuite.utils import http


async def test_ok(monitor_client):
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


def service_timestamp():
    return datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%SZ')


async def test_invalid_config(
        service_client, monitor_client, mockserver, dynamic_config,
):
    configs = dynamic_config.get_values()
    configs['HTTP_CLIENT_CONNECTION_POOL_SIZE'] = '1000'

    @mockserver.json_handler('configs-service/configs/values')
    async def mock_configs_values(request):
        return {'configs': configs, 'updated_at': service_timestamp()}

    assert await monitor_client.fired_alerts() == []

    try:
        await service_client.update_server_state()
        assert False
    except http.HttpResponseError:
        pass

    alerts = await monitor_client.fired_alerts()
    validate_alerts(alerts)

    configs['HTTP_CLIENT_CONNECTION_POOL_SIZE'] = 1000
    await service_client.update_server_state()
    assert await monitor_client.fired_alerts() == []


@pytest.mark.now('2019-01-01T12:00:00+0000')
async def test_alert_expiration(
        service_client,
        monitor_client,
        mockserver,
        mocked_time,
        dynamic_config,
):
    configs = dynamic_config.get_values()
    configs['HTTP_CLIENT_CONNECTION_POOL_SIZE'] = '1000'

    @mockserver.json_handler('configs-service/configs/values')
    async def mock_configs_values(request):
        return {'configs': configs, 'updated_at': service_timestamp()}

    assert await monitor_client.fired_alerts() == []

    try:
        await service_client.update_server_state()
        assert False
    except http.HttpResponseError:
        pass

    alerts = await monitor_client.fired_alerts()
    validate_alerts(alerts)

    mocked_time.sleep(119)
    await service_client.tests_control(invalidate_caches=False)
    alerts = await monitor_client.fired_alerts()
    validate_alerts(alerts)

    mocked_time.sleep(2)
    await service_client.tests_control(invalidate_caches=False)
    assert await monitor_client.fired_alerts() == []
