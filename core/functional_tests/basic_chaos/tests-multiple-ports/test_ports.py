import aiohttp


async def test_multiple_listen(service_client, port1, port2):
    # test port from listener.port
    response = await service_client.get('/ping')
    assert response.status_code == 200

    # test ports from listener.ports
    async with aiohttp.ClientSession() as session:
        response = await session.get(f'http://localhost:{port1}/ping')
        assert response.status == 200

        response = await session.get(f'http://localhost:{port2}/ping')
        assert response.status == 200
