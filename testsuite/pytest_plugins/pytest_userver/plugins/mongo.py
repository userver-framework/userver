"""
Plugin that imports the required fixtures to start the database
and adjusts the mongo "dbconnection" static config value.
"""

import re

import pytest


pytest_plugins = [
    'testsuite.databases.mongo.pytest_plugin',
    'pytest_userver.plugins.core',
]


USERVER_CONFIG_HOOKS = ['userver_mongo_config']


@pytest.fixture(scope='session')
def userver_mongo_config(mongo_connection_info):
    """
    Returns a function that adjusts the static configuration file for
    the testsuite.
    Sets the `dbconnection` to the testsuite started MongoDB credentials if
    the `dbconnection` starts with `mongodb://`. Additionally
    increases MongoDB connection timeouts to 30 seconds.

    @ingroup userver_testsuite_fixtures
    """

    port = mongo_connection_info.port
    host = mongo_connection_info.host
    new_uri = f'mongodb://{host}:{port}/'

    def _patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        for _, params in components.items():
            if params and 'dbconnection' in params:
                uri = params['dbconnection']
                if not uri.startswith('mongodb://'):
                    continue

                uri = re.sub('mongodb://[^:]+:\\d+/', new_uri, uri)
                uri = re.sub('mongodb://[^:]+/', new_uri, uri)

                params['dbconnection'] = uri
                params['conn_timeout'] = '30s'
                params['so_timeout'] = '30s'

    return _patch_config
