import pytest


@pytest.mark.ydb(files=['fill_events.sql'])
async def test_select_rows(service_client):
    response = await service_client.post('ydblib/select-list')
    assert response.status_code == 200
    assert response.json() == {
        'channels': [1, 2],
        'names': ['name-1', 'name-2'],
    }
