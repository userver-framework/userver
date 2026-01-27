import logging

import pytest

logger = logging.getLogger('test')

USERVER_LOGGED_TEXT = 'headers complete'
SERVICE_LOGGED_TEXT = 'Everything is OK'

try:
    import yatest.common  # noqa: F401

    # arcadia
    USERVER_LOCATION_FILE = 'taxi/uservices/userver/core/src/server/http/http_request_parser.cpp'
    SERVICE_LOCATION_BAD = 'taxi/uservices/userver/core/src/server/handlers/ping.cpp:99'
    SERVICE_LOCATION = 'taxi/uservices/userver/core/src/server/handlers/ping.cpp:100'
    PREFIX = 'taxi/'
except ModuleNotFoundError:
    # cmake
    USERVER_LOCATION_FILE = 'userver/core/src/server/http/http_request_parser.cpp'
    SERVICE_LOCATION_BAD = 'userver/core/src/server/handlers/ping.cpp:99'
    SERVICE_LOCATION = 'userver/core/src/server/handlers/ping.cpp:100'
    PREFIX = 'userver/'


SERVICE_LOCATION_DIR = SERVICE_LOCATION.rsplit('/', 1)[0]


@pytest.fixture(name='taxi_bodyless_headers_mock')
def _taxi_bodyless_headers_mock(mockserver):
    @mockserver.json_handler(
        '/test-service/response-with-headers-without-body',
    )
    async def _handler(request):
        return mockserver.make_response(headers={'something': 'nothing'})


async def test_service_debug_logs_off(
    service_client,
    taxi_bodyless_headers_mock,
):
    async with service_client.capture_logs() as capture:
        response = await service_client.get('/ping')
        assert response.status_code == 200

    assert not capture.select(text=SERVICE_LOGGED_TEXT)


async def test_service_debug_logs_on(
    service_client,
    monitor_client,
    taxi_bodyless_headers_mock,
):
    resp = await monitor_client.put(
        '/log/dynamic-debug',
        params={'location': SERVICE_LOCATION},
    )
    assert resp.status_code == 200

    async with service_client.capture_logs() as capture:
        response = await service_client.get('/ping')
        assert response.status_code == 200

    assert capture.select(text=SERVICE_LOGGED_TEXT)


async def test_service_debug_logs_on_directory(
    service_client,
    monitor_client,
    taxi_bodyless_headers_mock,
):
    resp = await monitor_client.put(
        '/log/dynamic-debug',
        params={'location': SERVICE_LOCATION_DIR},
    )
    assert resp.status_code == 200

    async with service_client.capture_logs() as capture:
        response = await service_client.get('/ping')
        assert response.status_code == 200

    assert capture.select(text=SERVICE_LOGGED_TEXT)

    resp = await monitor_client.delete(
        '/log/dynamic-debug',
        params={'location': SERVICE_LOCATION_DIR},
    )
    assert resp.status_code == 200

    async with service_client.capture_logs() as capture:
        response = await service_client.get('/ping')
        assert response.status_code == 200

    assert not capture.select(text=SERVICE_LOGGED_TEXT)


async def test_invalid_line(
    service_client,
    monitor_client,
    taxi_bodyless_headers_mock,
):
    resp = await monitor_client.put(
        '/log/dynamic-debug',
        params={'location': SERVICE_LOCATION_BAD},
    )
    assert resp.status_code == 500


async def test_service_debug_logs_prefix(
    service_client,
    monitor_client,
    taxi_bodyless_headers_mock,
):
    resp = await monitor_client.get('/log/dynamic-debug')

    assert resp.status_code == 200
    lines = resp.text.split('\n')
    for line in lines:
        if len(line) == 0:
            continue
        path, _ = line.split()
        logger.info(path)
        assert path.startswith(PREFIX)


async def test_service_debug_logs_on_off(
    service_client,
    monitor_client,
    taxi_bodyless_headers_mock,
):
    resp = await monitor_client.put(
        '/log/dynamic-debug',
        params={'location': SERVICE_LOCATION},
    )
    assert resp.status_code == 200
    resp = await monitor_client.delete(
        '/log/dynamic-debug',
        params={'location': SERVICE_LOCATION},
    )
    assert resp.status_code == 200

    async with service_client.capture_logs() as capture:
        response = await service_client.get('/ping')
        assert response.status_code == 200

    assert not capture.select(text=SERVICE_LOGGED_TEXT)


async def test_userver_debug_logs_file_off(
    service_client,
    taxi_bodyless_headers_mock,
):
    async with service_client.capture_logs() as capture:
        response = await service_client.get('/ping')
        assert response.status_code == 200

    assert not capture.select(text=USERVER_LOGGED_TEXT)


async def test_userver_debug_logs_file(
    service_client,
    monitor_client,
    taxi_bodyless_headers_mock,
):
    resp = await monitor_client.put(
        '/log/dynamic-debug',
        params={'location': USERVER_LOCATION_FILE},
    )
    assert resp.status_code == 200

    async with service_client.capture_logs() as capture:
        response = await service_client.get('/ping')
        assert response.status_code == 200

    assert capture.select(text=USERVER_LOGGED_TEXT)


async def test_userver_dynamic_config_debug_logs_on(
    service_client,
    dynamic_config,
    monitor_client,
    taxi_bodyless_headers_mock,
):
    dynamic_config.set_values({
        'USERVER_LOG_DYNAMIC_DEBUG': {
            'force-enabled': ['bad-value', SERVICE_LOCATION, 'another-bad-value'],
            'force-disabled': [],
        },
    })

    async with service_client.capture_logs() as capture:
        response = await service_client.get('/ping')
        assert response.status_code == 200

    assert capture.select(text=SERVICE_LOGGED_TEXT)


async def test_userver_dynamic_config_debug_logs_file_on(
    service_client,
    dynamic_config,
    monitor_client,
    taxi_bodyless_headers_mock,
):
    dynamic_config.set_values({
        'USERVER_LOG_DYNAMIC_DEBUG': {
            'force-enabled': ['bad-value', USERVER_LOCATION_FILE, 'another-bad-value'],
            'force-disabled': [],
        },
    })

    async with service_client.capture_logs() as capture:
        response = await service_client.get('/ping')
        assert response.status_code == 200

    assert capture.select(text=USERVER_LOGGED_TEXT)


async def test_userver_dynamic_config_debug_logs_dirs_ignored(
    service_client,
    dynamic_config,
    monitor_client,
    taxi_bodyless_headers_mock,
):
    dynamic_config.set_values({
        'USERVER_LOG_DYNAMIC_DEBUG': {
            'force-enabled': [SERVICE_LOCATION_DIR],
            'force-disabled': [],
        },
    })

    async with service_client.capture_logs() as capture:
        response = await service_client.get('/ping')
        assert response.status_code == 200

    assert not capture.select(text=USERVER_LOGGED_TEXT)


async def test_userver_dynamic_config_debug_logs_level_on(
    service_client,
    dynamic_config,
    monitor_client,
    taxi_bodyless_headers_mock,
):
    dynamic_config.set_values({
        'USERVER_LOG_DYNAMIC_DEBUG': {
            'force-enabled': [],
            'force-disabled': [],
            'force-enabled-level': {
                'bad-value': 'TRACE',
                SERVICE_LOCATION: 'TRACE',
                'another-bad-value': 'ERROR',
            },
            'force-disabled-level': {},
        },
    })
    async with service_client.capture_logs() as capture:
        response = await service_client.get('/ping')
        assert response.status_code == 200
    assert capture.select(text=SERVICE_LOGGED_TEXT)

    dynamic_config.set_values({
        'USERVER_LOG_DYNAMIC_DEBUG': {
            'force-enabled': [],
            'force-disabled': [],
            'force-enabled-level': {},
            'force-disabled-level': {},
        },
    })
    async with service_client.capture_logs() as capture:
        response = await service_client.get('/ping')
        assert response.status_code == 200
    assert not capture.select(text=SERVICE_LOGGED_TEXT)


async def test_userver_dynamic_config_debug_logs_off(
    service_client,
    dynamic_config,
    monitor_client,
    taxi_bodyless_headers_mock,
):
    resp = await monitor_client.put(
        '/log/dynamic-debug',
        params={'location': SERVICE_LOCATION},
    )
    assert resp.status_code == 200

    dynamic_config.set_values({
        'USERVER_LOG_DYNAMIC_DEBUG': {
            'force-enabled': [],
            'force-disabled': [SERVICE_LOCATION],
        },
    })

    async with service_client.capture_logs() as capture:
        response = await service_client.get('/ping')
        assert response.status_code == 200

    assert not capture.select(text=SERVICE_LOGGED_TEXT)


async def test_userver_dynamic_config_debug_logs_file_off(
    service_client,
    dynamic_config,
    monitor_client,
    taxi_bodyless_headers_mock,
):
    resp = await monitor_client.put(
        '/log/dynamic-debug',
        params={'location': USERVER_LOCATION_FILE},
    )
    assert resp.status_code == 200

    dynamic_config.set_values({
        'USERVER_LOG_DYNAMIC_DEBUG': {
            'force-enabled': [],
            'force-disabled': [USERVER_LOCATION_FILE],
        },
    })

    async with service_client.capture_logs() as capture:
        response = await service_client.get('/ping')
        assert response.status_code == 200

    assert not capture.select(text=USERVER_LOGGED_TEXT)


async def test_userver_dynamic_config_debug_logs_level_off(
    service_client,
    dynamic_config,
    monitor_client,
    taxi_bodyless_headers_mock,
):
    resp = await monitor_client.put(
        '/log/dynamic-debug',
        params={'location': SERVICE_LOCATION},
    )
    assert resp.status_code == 200

    dynamic_config.set_values({
        'USERVER_LOG_DYNAMIC_DEBUG': {
            'force-enabled': [],
            'force-disabled': [],
            'force-enabled-level': {},
            'force-disabled-level': {SERVICE_LOCATION: 'TRACE'},
        },
    })

    async with service_client.capture_logs() as capture:
        response = await service_client.get('/ping')
        assert response.status_code == 200

    assert not capture.select(text=SERVICE_LOGGED_TEXT)
