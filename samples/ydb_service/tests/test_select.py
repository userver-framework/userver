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


@pytest.mark.skip(reason='restore in TAXICOMMON-8860')
async def test_select_rows_empty(service_client):
    response = await service_client.post(
        'ydb/select-rows',
        json={
            'service': 'srv',
            'channels': [1, 2, 3],
            'created': '2019-10-30T11:20:00+00:00',
        },
    )
    assert response.status_code == 200
    assert response.json() == {'items': []}


@pytest.mark.ydb(files=['fill_events.sql'])
async def test_select_rows_with_state(service_client):
    response = await service_client.post(
        'ydb/select-rows',
        json={
            'service': 'srv-state',
            'channels': [1, 2, 3],
            'created': '2019-10-30T11:20:00+00:00',
        },
    )
    assert response.status_code == 200
    assert response.json() == {
        'items': [
            {
                'id': 'id-2',
                'name': 'name-2',
                'service': 'srv-state',
                'channel': 2,
                'created': '2019-10-31T11:20:00+00:00',
                'state': {'qwe': 'asd', 'zxc': 123},
            },
        ],
    }
