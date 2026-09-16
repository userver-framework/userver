import asyncio
import json
import os


def _valid_dsn(pgsql_local):
    database = pgsql_local['key_value']
    return (
        f'Driver={{PostgreSQL Unicode}};Server={database.host};'
        f'Port={database.port};Database={database.dbname};'
        f'Uid={database.user or "testsuite"};Pwd={database.password or ""};'
    )


def _replace_secdist(path, dsn):
    temporary = path.with_suffix('.tmp')
    temporary.write_text(
        json.dumps({
            'odbc_settings': {
                'databases': {'odbc-test': {'dsn': dsn}},
            },
        }),
    )
    os.replace(temporary, path)


async def test_secdist_hot_reload(service_client, secdist_path, pgsql_local, testpoint):
    failed = await service_client.get('/odbc')
    assert failed.status == 500

    @testpoint('odbc-new-dsn-list')
    def new_dsn_list(_data):
        pass

    await service_client.update_server_state()

    # Periodic secdist notifications with unchanged data must not rebuild all
    # pools or reset their metrics.
    await asyncio.sleep(1.2)
    assert new_dsn_list.times_called == 0

    _replace_secdist(secdist_path, _valid_dsn(pgsql_local))
    await new_dsn_list.wait_call(timeout=10)

    for _ in range(20):
        response = await service_client.get('/odbc')
        if response.status == 200:
            assert response.text == '42'
            return
        await asyncio.sleep(0.1)

    raise AssertionError('cached ODBC cluster did not recover after secdist update')
