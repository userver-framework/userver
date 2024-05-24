import bson
import pytest


DATA = {'value': b'\x00\x01\x02\x03'}

SQL_REQUEST = """
--!syntax_v1
SELECT doc FROM `orders`
WHERE id = "id1"
"""


@pytest.mark.skip(reason='restore in TAXICOMMON-8860')
async def test_ok(service_client, ydb):
    # validate YDB state
    cursor = ydb.execute(SQL_REQUEST)
    assert len(cursor) == 1
    assert not cursor[0].rows
    # perform request
    response = await service_client.post(
        'ydb/bson-upserting',
        params={'id': 'id1'},
        data=bson.BSON.encode(DATA),
        headers={'Content-Type': 'application/bson'},
    )
    # validate response
    assert response.status_code == 200
    # validate YDB state
    cursor = ydb.execute(SQL_REQUEST)
    assert len(cursor) == 1
    assert len(cursor[0].rows) == 1
    assert DATA == bson.BSON.decode(cursor[0].rows[0]['doc'])
