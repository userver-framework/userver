# /// [Functional test]
async def test_config(service_client):
    response = await service_client.post('/configs/values', json={})
    assert response.status == 200
    reply = response.json()
    assert reply['configs']['USERVER_LOG_REQUEST_HEADERS'] is True


async def test_config_specific_ids(service_client):
    response = await service_client.post(
        '/configs/values',
        json={
            'updated_since': '2021-06-29T14:15:31.173239295+0000',
            'ids': ['USERVER_TASK_PROCESSOR_QOS'],
        },
    )
    assert response.status == 200
    reply = response.json()
    assert len(reply['configs']) == 1
    assert reply['configs']['USERVER_TASK_PROCESSOR_QOS']
    # /// [Functional test]
