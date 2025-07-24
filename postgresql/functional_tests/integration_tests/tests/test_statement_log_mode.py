import pytest


@pytest.fixture(scope='session')
def userver_pg_log_config():
    def patch_config(config_yaml, _config_vars) -> None:
        components = config_yaml['components_manager']['components']
        db = components['key-value-database']
        db['statement-log-mode'] = 'hide'

    return patch_config


@pytest.mark.uservice_oneshot(config_hooks=['userver_pg_log_config'])
async def test_make_request(service_client):
    async with service_client.capture_logs() as capture:
        response = await service_client.post('/v1/key-value?key=foo&value=bar')
        assert response.status in (200, 201)

    records = capture.select(stopwatch_name='pg_query')
    assert len(records) == 1, capture.select()
    assert 'db_statement' not in records[0]
