"""
Plugin that imports the required fixtures to start the Redis database.
"""

pytest_plugins = [
    'testsuite.databases.redis.pytest_plugin',
    'pytest_userver.plugins.core',
    'pytest_userver.plugins.redis_standalone.pytest_plugin'
]
