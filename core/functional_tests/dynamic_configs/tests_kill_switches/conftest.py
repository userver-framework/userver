import pytest

from . import constants

pytest_plugins = ['pytest_userver.plugins.core']


@pytest.fixture
def config_update_testpoint(testpoint):
    @testpoint('config-update')
    def config_update(_data):
        pass

    return config_update


@pytest.fixture(name='clean_cache_and_flush_testpoint')
async def _clean_cache_and_flush_testpoint(
    config_update_testpoint,
    service_client,
    dynamic_config,
):
    await service_client.update_server_state()
    config_update_testpoint.flush()


@pytest.fixture
async def check_fresh_update(
    service_client,
    config_update_testpoint,
    dynamic_config,
):
    async def check():
        await service_client.update_server_state()
        assert await config_update_testpoint.wait_call() == {
            '_data': dynamic_config.get(constants.CONFIG_KEY),
        }

    return check


@pytest.fixture
async def check_no_update(service_client, config_update_testpoint):
    async def check():
        await service_client.update_server_state()
        assert config_update_testpoint.times_called == 0

    return check


@pytest.fixture
async def check_reset_to_default(
    service_client,
    config_update_testpoint,
):
    async def check():
        await service_client.update_server_state()

        default_config = await service_client.get_dynamic_config_defaults()
        static_default = default_config[constants.CONFIG_KEY]
        assert await config_update_testpoint.wait_call() == {
            '_data': static_default,
        }

    return check
