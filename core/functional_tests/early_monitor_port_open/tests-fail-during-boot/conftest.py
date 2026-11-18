import pytest
import pytest_userver.config

pytest_plugins = ['pytest_userver.plugins.core']


@pytest.fixture(scope='session')
@pytest_userver.config.patch
def config_break():
    def patch_config(config_yaml, config_vars) -> None:
        components = config_yaml['components_manager']['components']
        components['handler-ping']['task_processor'] = 'not-existing-task-processor'
        components['brake'] = {
            'load-enabled': False,
        }

    return patch_config
