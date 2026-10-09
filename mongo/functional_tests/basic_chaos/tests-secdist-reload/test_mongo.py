import json
import os


async def rewrite_secdist(broken_secdist, mongo_connection_info, database='admin'):
    with open(broken_secdist + '.tmp', 'w') as ofile:
        ofile.write(
            json.dumps({
                'mongo_settings': {
                    'mongo-test': {
                        'uri': f'mongodb://{mongo_connection_info.host}:{mongo_connection_info.port}/{database}',
                    },
                },
            }),
        )
    os.rename(broken_secdist + '.tmp', broken_secdist)


async def test_secdist_update(
    service_client,
    broken_secdist,
    mongo_connection_info,
    testpoint,
):
    @testpoint('mongo-new-connection-string')
    def tp(request):
        pass

    await service_client.update_server_state()

    response = await service_client.put('/v1/key-value?key=foo&value=bar')
    assert response.status == 500

    await rewrite_secdist(broken_secdist, mongo_connection_info)
    await tp.wait_call()

    response = await service_client.put('/v1/key-value?key=foo&value=bar')
    assert response.status == 200

    await rewrite_secdist(broken_secdist, mongo_connection_info, 'userver_secdist_reload')
    await tp.wait_call()
    response = await service_client.get('/v1/key-value?key=foo')
    assert response.status == 404

    response = await service_client.put('/v1/key-value?key=foo&value=new-database')
    assert response.status == 200
    response = await service_client.get('/v1/key-value?key=foo')
    assert response.status == 200
    assert response.text == 'new-database'

    await rewrite_secdist(broken_secdist, mongo_connection_info)
    await tp.wait_call()
    response = await service_client.get('/v1/key-value?key=foo')
    assert response.status == 200
    assert response.text == 'bar'
