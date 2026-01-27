import pytest


async def test_file_not_found(service_client):
    response = await service_client.get('/file.not')
    assert response.status == 404
    assert b'File not found' in response.content


@pytest.mark.parametrize('path', ['/index.html', '/'])
async def test_file(service_client, service_source_dir, path):
    response = await service_client.get(path)
    assert response.status == 200
    assert response.headers['Content-Type'] == 'text/html'
    assert response.headers['Expires'] == '600'
    file = service_source_dir.joinpath('public') / 'index.html'
    assert response.content.decode() == file.open().read()


async def test_file_recursive(service_client, service_source_dir):
    response = await service_client.get('/dir1/dir2/data.html')
    assert response.status == 200
    assert response.headers['Content-Type'] == 'text/html'
    assert response.content == b'file in recurse dir\n'
    file = service_source_dir.joinpath('public') / 'dir1' / 'dir2' / 'data.html'
    assert response.content.decode() == file.open().read()


@pytest.mark.parametrize('path', ['/dir1/dir2', '/dir1/dir2/'])
async def test_file_recursive_index(service_client, service_source_dir, path):
    response = await service_client.get(path)
    assert response.status == 200
    assert response.headers['Content-Type'] == 'text/html'
    file = service_source_dir.joinpath('public') / 'dir1' / 'dir2' / 'index.html'
    assert response.content.decode() == file.open().read()


async def test_hidden_file(service_client, service_source_dir):
    response = await service_client.get('/dir1/.hidden_file.txt')
    assert response.status == 404
    file = service_source_dir.joinpath('public') / '404.html'
    assert response.content.decode() == file.open().read()
