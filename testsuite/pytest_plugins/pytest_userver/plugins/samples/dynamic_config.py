import json

import pytest

USERVER_CONFIG_HOOKS = ['userver_config_dynconf_cache']


@pytest.fixture(scope='session')
def userver_config_dynconf_cache(service_tmpdir, config_service_defaults):
    def patch_config(config, _config_vars) -> None:
        cache_path = service_tmpdir / 'configs' / 'config_cache.json'

        if cache_path.is_file():
            # To avoid leaking dynamic config values between test sessions
            cache_path.unlink()

        components = config['components_manager']['components']
        client_updater = components.get('dynamic-config-client-updater', None)

        if client_updater:
            # Service is started at the 'session' level, before
            # 'function'-level '_mock_configs_service' fixture starts.
            # As a workaround, we provide initial config values through
            # dynamic config cache. Periodic config updates will fail, though.
            #
            # TODO find a way to perform initial config update, and perhaps
            #  random updates between tests, through the normal updates.
            cache_path.parent.mkdir(parents=True, exist_ok=True)
            with cache_path.open('w+', encoding='utf-8') as file:
                json.dump(config_service_defaults, file)
            components['dynamic-config']['fs-cache-path'] = str(cache_path)

    return patch_config
