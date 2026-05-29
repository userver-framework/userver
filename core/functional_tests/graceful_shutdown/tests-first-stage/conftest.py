import pytest

pytest_plugins = ['pytest_userver.plugins.core']


# Overriding a userver fixture
@pytest.fixture(name='userver_config_testsuite', scope='session')
def _userver_config_testsuite(userver_config_testsuite):
    def patch_config(config, config_vars) -> None:
        userver_config_testsuite(config, config_vars)
        # Restore the option after it's deleted by the base fixture.
        # Don't do this in your testsuite tests! For userver tests only.
        config['components_manager']['graceful_shutdown_continue_accepting_requests_interval'] = '3s'
        config['components_manager']['graceful_shutdown_pending_requests_completion_interval'] = '0s'

    return patch_config
