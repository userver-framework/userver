import pytest


@pytest.mark.ydb(files=['fill_events.sql'])
async def test_describe_table(service_client):
    response = await service_client.post(
        'ydb/describe-table', json={'path': 'events'},
    )
    assert response.status_code == 200
    assert response.json() == {'key_columns': ['id', 'name']}
