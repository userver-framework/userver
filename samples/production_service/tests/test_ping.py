async def test_ping(service_client):
    response = await service_client.get('/ping')
    assert response.status == 200

    # Tracing headers should be present
    assert 'X-YaRequestId' in response.headers.keys()
    assert 'X-YaTraceId' in response.headers.keys()
    assert 'X-YaSpanId' in response.headers.keys()
