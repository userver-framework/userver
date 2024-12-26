import json

import pytest

pytest_plugins = ['pytest_userver.plugins.postgresql']


USERVER_CONFIG_HOOKS = ['userver_pg_broken']


@pytest.fixture(scope='session')
def broken_secdist(service_tmpdir):
    with open(service_tmpdir / 'secdist.json', 'w') as ofile:
        ofile.write(
            json.dumps({
                'postgresql_settings': {
                    'databases': {
                        'pg-test': [
                            {
                                'shard_number': 0,
                                'hosts': ['postgresql:///localhost:0/'],
                            },
                        ],
                    },
                },
            }),
        )
        ofile.flush()
        yield ofile.name


@pytest.fixture(scope='session')
def userver_pg_broken(broken_secdist):
    def _hook_db_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']

        secdist = components['secdist']
        secdist['load-enabled'] = True
        secdist['update-period'] = '1s'

        components['default-secdist-provider']['config'] = broken_secdist

        db = components['key-value-database']
        db['dbalias'] = 'pg-test'
        db['sync-start'] = False
        db['connlimit_mode'] = 'manual'
        del db['dbconnection']

    return _hook_db_config
