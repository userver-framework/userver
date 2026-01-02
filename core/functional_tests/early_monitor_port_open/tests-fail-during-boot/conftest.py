import pytest

pytest_plugins = ['pytest_userver.plugins.core']

USERVER_CONFIG_HOOKS = ['config_break']


@pytest.fixture(scope='session')
def config_break():
    def patch_config(config_yaml, config_vars) -> None:
        components = config_yaml['components_manager']['components']
        components['handler-ping']['task_processor'] = 'not-existing-task-processor'
        components['brake'] = {
            'load-enabled': False,
        }

    return patch_config
