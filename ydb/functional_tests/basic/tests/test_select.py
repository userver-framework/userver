import pytest


@pytest.mark.ydb(files=['fill_events.sql'])
async def test_select_rows(service_client):
    response = await service_client.post(
        'ydb/select-rows',
        json={
            'service': 'srv',
            'channels': [1, 2, 3],
            'created': '2019-10-30T11:20:00+00:00',
        },
    )
    assert response.status_code == 200
    assert response.json() == {
        'items': [
            {
                'id': 'id-1',
                'name': 'name-1',
                'service': 'srv',
                'channel': 1,
                'created': '2019-10-31T11:20:00+00:00',
            },
        ],
    }
