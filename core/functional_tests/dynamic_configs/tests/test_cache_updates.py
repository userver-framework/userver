import json
import pathlib

import pytest

from testsuite.utils import http


def _load_dynamic_config_cache(service_config):
    cache_path = pathlib.Path(service_config['components_manager']['components']['dynamic-config']['fs-cache-path'])
    return json.loads(cache_path.read_text())


async def test_inc_update(service_client, service_config, testpoint):
    @testpoint('reset-cache-dynamic-config-client-updater')
    def tp_reset(data):
        pass

    await service_client.update_server_state()

    assert tp_reset.times_called <= 1
    if tp_reset.times_called == 1:
        assert tp_reset.next_call() == {'data': {'update_type': 'incremental'}}

    await service_client.invalidate_caches(
        cache_names=['dynamic-config-client-updater'],
    )

    assert tp_reset.times_called == 1
    assert tp_reset.next_call() == {'data': {'update_type': 'full'}}

    cache = _load_dynamic_config_cache(service_config)
    assert cache == {'HTTP_CLIENT_CONNECTION_POOL_SIZE': 777}


async def test_invalid_update_is_not_cached(service_client, service_config, dynamic_config):
    await service_client.update_server_state()
    cache_before = _load_dynamic_config_cache(service_config)
    original_value = dynamic_config.get('HTTP_CLIENT_CONNECTION_POOL_SIZE')

    dynamic_config.set(HTTP_CLIENT_CONNECTION_POOL_SIZE='invalid')
    with pytest.raises(http.HttpResponseError):
        await service_client.update_server_state()

    assert _load_dynamic_config_cache(service_config) == cache_before

    # Bypass deduplication to clear the parse error before restoring the original value.
    dynamic_config.set(HTTP_CLIENT_CONNECTION_POOL_SIZE=original_value + 1)
    await service_client.update_server_state()
    dynamic_config.set(HTTP_CLIENT_CONNECTION_POOL_SIZE=original_value)
    await service_client.update_server_state()
