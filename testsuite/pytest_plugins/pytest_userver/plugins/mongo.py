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
    port = mongo_connection_info.port
    host = mongo_connection_info.host
    new_uri = f'mongodb://{host}:{port}/'

    def _patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        for _, params in components.items():
            if params and 'dbconnection' in params:
                params['dbconnection'] = re.sub(
                    'mongodb://[^:]+:\\d+/', new_uri, params['dbconnection'],
                )
                params['dbconnection'] = re.sub(
                    'mongodb://[^:]+/', new_uri, params['dbconnection'],
                )

    return _patch_config
