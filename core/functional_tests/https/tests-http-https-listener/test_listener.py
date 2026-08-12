import ssl

import aiohttp


async def test_http_https_listener(service_client, port1, port2, service_source_dir):
    async with aiohttp.ClientSession() as session:
        response = await session.get(f'http://localhost:{port1}/ping')
        assert response.status == 200

    ssl_context = ssl.create_default_context()
    ssl_context.load_verify_locations(str(service_source_dir / 'cert.crt'))
    connector = aiohttp.TCPConnector(ssl=ssl_context)
    async with aiohttp.ClientSession(connector=connector) as session:
        response = await session.get(f'https://localhost:{port2}/ping')
        assert response.status == 200
