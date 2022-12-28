import pytest


TEST_CONFIG = 'MEANING_OF_LIFE'
DEFAULT_VALUE = 42
CUSTOM_VALUE = 1337
SPECIAL_VALUE = 9001


async def get_service_config_value(service_client):
    response = await service_client.get('/dynamic-config')
    assert response.status == 200
    return int(response.text)


async def test_a_defaults(service_client, dynamic_config):
    assert dynamic_config.get(TEST_CONFIG) == DEFAULT_VALUE
    assert await get_service_config_value(service_client) == DEFAULT_VALUE


async def test_set_values(service_client, dynamic_config):
    dynamic_config.set_values({TEST_CONFIG: CUSTOM_VALUE})
    assert dynamic_config.get(TEST_CONFIG) == CUSTOM_VALUE
    # On first 'service_client' usage in the test, dynamic configs are loaded
    # automatically from the 'dynamic_config' fixture to the service.
    assert await get_service_config_value(service_client) == CUSTOM_VALUE

    # However, subsequent changes are not synchronized automatically.
    # TODO Always synchronize 'dynamic_config' with the service automatically.
    dynamic_config.set_values({TEST_CONFIG: SPECIAL_VALUE})
    assert dynamic_config.get(TEST_CONFIG) == SPECIAL_VALUE
    assert await get_service_config_value(service_client) == CUSTOM_VALUE

    # 'invalidate_caches' does the job of "uploading" updated configs.
    await service_client.invalidate_caches()
    assert await get_service_config_value(service_client) == SPECIAL_VALUE


async def test_z_defaults_again(service_client, dynamic_config):
    assert dynamic_config.get(TEST_CONFIG) == DEFAULT_VALUE
    assert await get_service_config_value(service_client) == DEFAULT_VALUE


@pytest.mark.config(MEANING_OF_LIFE=CUSTOM_VALUE)
async def test_annotation(service_client, dynamic_config):
    assert dynamic_config.get(TEST_CONFIG) == CUSTOM_VALUE
    assert await get_service_config_value(service_client)
