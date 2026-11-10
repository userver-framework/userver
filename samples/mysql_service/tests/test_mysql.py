import json


def test_secdist_uses_mysql_conninfo(service_env, mysql_conninfo):
    settings = json.loads(service_env['SECDIST_CONFIG'])['mysql_settings']['sample-sql-component']
    assert settings == {
        'hosts': [mysql_conninfo.hostname],
        'port': mysql_conninfo.port,
        'user': mysql_conninfo.user,
        'password': mysql_conninfo.password or '',
        'database': 'sample_db',
    }


async def test_basic(service_client):
    response = await service_client.post(
        '/v1/db/',
        json={'data': [{'key': 1, 'value': '1'}]},
    )
    assert response.status_code == 200

    response = await service_client.get('/v1/db/')
    assert response.status_code == 200
    assert 'application/json' in response.headers['Content-Type']
    assert response.json()['values'] == [{'key': 1, 'value': '1'}]


async def test_batch_insert(service_client):
    response = await service_client.post(
        '/v1/db/',
        json={'data': [{'key': i, 'value': str(i)} for i in range(10)]},
    )
    assert response.status_code == 200

    response = await service_client.get('/v1/db/')
    assert response.status_code == 200
    assert 'application/json' in response.headers['Content-Type']
    assert response.json()['values'] == [{'key': i, 'value': str(i)} for i in range(10)]
