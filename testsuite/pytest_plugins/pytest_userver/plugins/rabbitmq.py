"""
Plugin that imports the required fixtures to start the Redis database.
"""

pytest_plugins = [
    'testsuite.databases.rabbitmq.pytest_plugin',
    'pytest_userver.plugins.core',
]
