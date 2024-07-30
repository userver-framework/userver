import pytest


@pytest.mark.now('2019-01-01T12:00:00+0000')
async def test_hello_base(service_client):
    response = await service_client.post('/hello', json={})
    assert response.status == 200
    assert response.json() == {
        'text': 'Hello, noname!\n',
        'current-time': '2019-01-01T12:00:00+00:00',
    }

    response = await service_client.post('/hello', json={'name': 'userver'})
    assert response.status == 200
    assert response.json() == {
        'text': 'Hello, userver!\n',
        'current-time': '2019-01-01T12:00:00+00:00',
    }
