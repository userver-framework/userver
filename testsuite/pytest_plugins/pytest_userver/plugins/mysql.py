"""
Plugin that imports the required fixtures to start the MySQL/MariaDB database.
"""

pytest_plugins = [
    'testsuite.databases.mysql.pytest_plugin',
    'pytest_userver.plugins.core',
]
