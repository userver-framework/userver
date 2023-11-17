# See the corresponding dynamic_config_fallback_patch in the root conftest.py
async def test_dynamic_config_fallback_patch(service_client):
    defaults = await service_client.get_dynamic_config_defaults()
    assert defaults['HTTP_CLIENT_CONNECTION_POOL_SIZE'] == 777
