"""
Plugin that imports the required fixtures to start the Clickhouse database.
"""

pytest_plugins = [
    'testsuite.databases.clickhouse.pytest_plugin',
    'pytest_userver.plugins.core',
]
