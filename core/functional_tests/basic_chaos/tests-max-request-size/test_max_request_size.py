import gzip


async def test_request_size_success(service_client):
    response = await service_client.get('/ping')
    assert response.status == 200


async def test_request_size_fail(service_client):
    response = await service_client.get('/ping', params={'arg': 'x' * 1000})
    assert response.status == 413
    assert response.text == 'too large request'


async def test_compressed_body_size_success(service_client):
    data = gzip.compress(b'x' * 1000)
    assert len(data) < 200
    response = await service_client.post('/ping', data=data, headers={'Content-Encoding': 'gzip'})
    assert response.status == 200


async def test_compressed_body_size_fail(service_client):
    data = gzip.compress(b'x' * 1001)
    assert len(data) < 200
    response = await service_client.post('/ping', data=data, headers={'Content-Encoding': 'gzip'})
    assert response.status == 413
