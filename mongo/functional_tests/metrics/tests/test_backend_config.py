import pytest

from testsuite.daemons import spawn


@pytest.fixture(scope='session')
def experimental_without_backend():
    def patch_config(config_yaml, _config_vars):
        manager = config_yaml['components_manager']
        manager['userver_experiments']['mongo-thread-backend'] = False
        manager['components']['key-value-database']['driver'] = 'mongo-c-driver-experimental'

    return patch_config


@pytest.fixture(scope='session')
def legacy_without_backend():
    def patch_config(config_yaml, _config_vars):
        manager = config_yaml['components_manager']
        manager.pop('userver_experiments', None)
        manager['components']['key-value-database']['driver'] = 'mongo-c-driver'

    return patch_config


@pytest.mark.uservice_oneshot(config_hooks=['experimental_without_backend'])
async def test_experimental_pool_requires_backend(
    service_binary,
    service_config_path_temp,
    service_env,
    create_daemon_scope,
    ensure_daemon_started,
    service_health_check,
):
    stderr = []
    async with create_daemon_scope(
        args=[str(service_binary), '--config', str(service_config_path_temp)],
        env=service_env,
        health_check=service_health_check,
        stderr_handler=stderr.append,
    ) as scope:
        with pytest.raises((spawn.HealthCheckError, spawn.ExitCodeError)):
            await ensure_daemon_started(scope)
    assert any(b'Experimental MongoDB pool' in line for line in stderr)


@pytest.mark.uservice_oneshot(config_hooks=['legacy_without_backend'])
async def test_legacy_pool_without_backend(service_client):
    response = await service_client.put('/v1/key-value?key=backend-default&value=native')
    assert response.status == 200
    response = await service_client.get('/v1/key-value?key=backend-default')
    assert response.status == 200
    assert response.text == '1'
