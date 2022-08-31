async def test_file_not_found(service_client):
    response = await service_client.get('/file.not')
    assert response.status == 404
    assert response.content == b'File not found'


async def test_file(service_client, service_source_dir):
    response = await service_client.get('/index.html')
    assert response.status == 200
    assert response.headers['Content-Type'] == 'text/html'
    file = service_source_dir.joinpath('public') / 'index.html'
    assert response.content.decode() == file.open().read()


async def test_file_recursive(service_client, service_source_dir):
    response = await service_client.get('/dir1/dir2/data.html')
    assert response.status == 200
    assert response.headers['Content-Type'] == 'text/html'
    assert response.content == b'file in recurse dir\n'
    file = (
        service_source_dir.joinpath('public') / 'dir1' / 'dir2' / 'data.html'
    )
    assert response.content.decode() == file.open().read()


async def test_hidden_file(service_client, service_source_dir):
    response = await service_client.get('/dir1/.hidden_file.txt')
    assert response.status == 404
    assert response.content.decode() == 'File not found'
