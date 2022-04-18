import pytest


@pytest.mark.now('2019-12-31T11:22:33Z')
async def test_now(service_client):
    response = await service_client.get('/now')
    assert response.status == 200
    assert response.json() == {'now': '2019-12-31T11:22:33+00:00'}
