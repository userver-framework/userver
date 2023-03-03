import datetime

import pytest


_BASE_TIME_SEC = 100
_RECENT_PERIOD_WAIT_INTERVAL_SEC = 6
_DEFAULT_NOW = datetime.datetime.utcfromtimestamp(_BASE_TIME_SEC)

pytest_plugins = ['pytest_userver.plugins.mongo']

MONGO_COLLECTIONS = {
    'translations': {
        'settings': {
            'collection': 'translations',
            'connection': 'admin',
            'database': 'admin',
        },
        'indexes': [],
    },
}


@pytest.fixture(scope='session')
def mongodb_settings():
    return MONGO_COLLECTIONS


@pytest.fixture
async def force_mocked_time_for_metrics(service_client):
    # mocked_time is not automatically propagated to the service
    # when using service_monitor.
    await service_client.update_server_state()


@pytest.fixture(scope='session')
def _metrics_setup_once_flag() -> list:
    return []


@pytest.fixture
async def force_metrics_to_appear(
        service_client,
        mocked_time,
        _metrics_setup_once_flag,
        force_mocked_time_for_metrics,
):
    if _metrics_setup_once_flag:
        return
    _metrics_setup_once_flag.append(True)

    old_now = mocked_time.now()
    now = datetime.datetime.utcfromtimestamp(
        _BASE_TIME_SEC - _RECENT_PERIOD_WAIT_INTERVAL_SEC,
    )
    mocked_time.set(now)
    await service_client.update_server_state()

    response = await service_client.put('/v1/key-value?key=foo&value=bar')
    assert response.status == 200

    response = await service_client.get('/v1/key-value?key=foo')
    assert response.status == 200
    assert response.text == '1'

    mocked_time.set(old_now)
    await service_client.update_server_state()


def pytest_collection_modifyitems(items):
    for item in items:
        item.add_marker(pytest.mark.now(str(_DEFAULT_NOW)))
