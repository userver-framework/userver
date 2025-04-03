import json

import pytest

pytest_plugins = ['pytest_userver.plugins.mongo']


@pytest.fixture(scope='session')
def broken_secdist(service_tmpdir):
    with open(service_tmpdir / 'secdist.json', 'w') as ofile:
        ofile.write(
            json.dumps({
                'mongo_settings': {
                    'mongo-test': {'uri': 'mongodb://[::1]:10/admin'},
                },
            }),
        )
        ofile.flush()
        yield ofile.name


@pytest.fixture(scope='session')
def userver_mongo_config(broken_secdist):
    def _hook_db_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']

        secdist = components['secdist']
        secdist['load-enabled'] = True
        secdist['update-period'] = '1s'

        components['default-secdist-provider']['config'] = broken_secdist

        db = components['key-value-database']
        db['dbalias'] = 'mongo-test'

    return _hook_db_config
