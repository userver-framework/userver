import json
import os

import pytest


def fix_secdist(service_tmpdir, pgsql_local):
    uri = pgsql_local['key_value'].get_uri()

    with open(service_tmpdir / 'secdist.json.tmp', 'w') as ofile:
        ofile.write(
            json.dumps({
                'postgresql_settings': {
                    'databases': {
                        'pg-test': [{'shard_number': 0, 'hosts': [uri]}],
                    },
                },
            }),
        )
    os.rename(
        service_tmpdir / 'secdist.json.tmp',
        service_tmpdir / 'secdist.json',
    )


def break_secdist(service_tmpdir) -> str:
    with open(service_tmpdir / 'secdist.json', 'w') as ofile:
        ofile.write(
            json.dumps({
                'postgresql_settings': {
                    'databases': {
                        'pg-test': [
                            {
                                'shard_number': 0,
                                'hosts': ['postgresql:///localhost:1/'],
                            },
                        ],
                    },
                },
            }),
        )
    return str(service_tmpdir / 'secdist.json')


@pytest.fixture(scope='session')
def broken_secdist(service_tmpdir):
    yield break_secdist(service_tmpdir)


@pytest.fixture(scope='session')
def userver_pg_config(broken_secdist):
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

    return _hook_db_config


async def test_update(service_client, service_tmpdir, pgsql_local, testpoint):
    @testpoint('postgres-new-dsn-list')
    def tp(request):
        pass

    break_secdist(service_tmpdir)

    while True:
        response = await service_client.get(
            '/postgres?type=select-small-timeout',
        )
        if response.status == 500:
            break

    fix_secdist(service_tmpdir, pgsql_local)
    await tp.wait_call()

    response = await service_client.get('/postgres?type=select-small-timeout')
    assert response.status == 200


async def test_no_memleak(
    service_client,
    service_tmpdir,
    pgsql_local,
    testpoint,
):
    @testpoint('postgres-new-dsn-list')
    async def tp(request):
        pass

    fix_secdist(service_tmpdir, pgsql_local)

    while True:
        response = await service_client.get('/postgres')
        if response.status == 200:
            break

    @testpoint('pg-call')
    async def tp_call(request):
        break_secdist(service_tmpdir)

        # wait until secdist is broken to continue pg activity with old connection
        await tp.wait_call()

    response = await service_client.get('/postgres?sleep=1')
    assert response.status == 200
