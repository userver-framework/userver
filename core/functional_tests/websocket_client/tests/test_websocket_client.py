import pytest


@pytest.mark.parametrize(
    'test_name',
    [
        'echo',
        'large',
        'multiple',
        'binary',
        'concurrent',
        'nonblocking_read',
        'nonblocking_write',
        'unauth',
        'connection_already_extracted',
    ],
)
async def test_client(service_client, service_port, test_name):
    response = await service_client.get(
        '/test-client',
        params={'test': test_name, 'port': service_port},
        timeout=30.0,
    )

    assert response.status == 200
    assert response.text == 'OK'
