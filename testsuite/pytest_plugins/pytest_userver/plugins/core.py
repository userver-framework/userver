"""
Python plugin that provides core fixtures for functional tests with
testsuite; see
@ref scripts/docs/en/userver/functional_testing.md for an introduction.

@ingroup userver_testsuite_fixtures
"""

import pytest

pytest_plugins = [
    'testsuite.pytest_plugin',
    'pytest_userver.plugins.asyncio_socket',
    'pytest_userver.plugins.base',
    'pytest_userver.plugins.caches',
    'pytest_userver.plugins.config',
    'pytest_userver.plugins.dumps',
    'pytest_userver.plugins.dynamic_config',
    'pytest_userver.plugins.log_capture',
    'pytest_userver.plugins.logging',
    'pytest_userver.plugins.service',
    'pytest_userver.plugins.service_client',
    'pytest_userver.plugins.service_runner',
    'pytest_userver.plugins.testpoint',
]


def pytest_configure(config: pytest.Config) -> None:
    try:
        # Makes sure userver's own tests run stably in the internal CI. Not for use by normal services.
        import pytest_userver.plugins.internal_overrides as plugin

        config.pluginmanager.register(plugin)
    except ImportError:
        pass
