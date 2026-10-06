import pytest


@pytest.fixture(scope='session')
def service_port(get_free_port) -> int:
    return get_free_port()


# /// [Functional test]
async def test_hello_base(service_client):
    response = await service_client.get('/hello')
    assert response.status == 200
    assert 'text/plain' in response.headers['Content-Type']
    assert response.text == 'Hello, unknown user!\n'
    assert 'X-RequestId' not in response.headers.keys(), 'Unexpected header'
    # /// [Functional test]
