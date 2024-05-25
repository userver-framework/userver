import bson
import pytest


@pytest.mark.skip(reason='restore in TAXICOMMON-8860')
@pytest.mark.ydb(files=['fill_orders.sql'])
async def test_bson_reading(service_client):
    response = await service_client.post(
        'ydb/bson-reading', params={'id': 'id1'},
    )
    assert response.status_code == 200
    raw_bson = bson.BSON(response.content).decode()
    res = raw_bson['doc']
    assert res == {
        'h': {'e': {'l': {'l': '0'}}},
        'w': {'o': {'r': {'l': 'd'}}},
    }
