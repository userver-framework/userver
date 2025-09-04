# [Functional test]
import pytest


@pytest.mark.parametrize('path', ['/index.html', '/'])
async def test_file(service_client, service_source_dir, path):
    response = await service_client.get(path)
    assert response.status == 200
    assert response.headers['Content-Type'] == 'text/html'
    assert response.headers['Expires'] == '600'
    file = service_source_dir.joinpath('public') / 'index.html'
    assert response.content.decode() == file.open().read()
    # [Functional test]


async def test_file_not_found(service_client):
    response = await service_client.get('/file.not')
    assert response.status == 404
    assert b'File not found' in response.content
