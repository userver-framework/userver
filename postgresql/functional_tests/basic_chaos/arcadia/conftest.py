import os

import pytest

import yatest.common  # pylint: disable=import-error

USERVER_CONFIG_HOOKS = ['hook_db_config']


@pytest.fixture(scope='session')
def service_binary():
    path = os.path.relpath(
        yatest.common.test_source_path(
            '../userver-postgresql-tests-basic-chaos',
        ),
        yatest.common.source_path(),
    )
    return yatest.common.binary_path(path)


@pytest.fixture(scope='session')
def hook_db_config(pgsql_local, for_clinet_gate_port):
    def _hook_db_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        db = components['key-value-database']
        db['dbconnection'] = (
            'postgresql://testsuite@localhost:'
            + str(for_clinet_gate_port)
            + '/pg_key_value'
        )

    return _hook_db_config
