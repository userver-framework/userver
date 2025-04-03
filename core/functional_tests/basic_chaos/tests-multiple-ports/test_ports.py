import aiohttp


async def test_multiple_listen(service_client, port1, port2):
    async with aiohttp.ClientSession() as session:
        response = await session.get(f'http://localhost:{port1}/ping')
        assert response.status == 200

        response = await session.get(f'http://localhost:{port2}/ping')
        assert response.status == 200
