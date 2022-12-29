"""
Helpers for the userver samples and functional tests.

These are NOT included into pytest_userver.plugins.
"""

pytest_plugins = [
    'pytest_userver.plugins.samples.base',
    'pytest_userver.plugins.samples.dynamic_config',
]
