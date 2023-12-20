import pathlib
import ssl

import aiohttp

SERVICE_SOURCE_DIR = pathlib.Path(__file__).parent.parent


def make_conn(cert=True):
    ssl_ctx = ssl.create_default_context(
        cafile=str(SERVICE_SOURCE_DIR / 'cert.crt'),
    )
    if cert:
        ssl_ctx.load_cert_chain(
            str(SERVICE_SOURCE_DIR / 'cert.crt'),
            str(SERVICE_SOURCE_DIR / 'private_key.key'),
        )

    conn = aiohttp.TCPConnector(ssl=ssl_ctx)
    return conn


# /// [Functional test]
async def test_cert_ok(service_client, service_port):
    async with aiohttp.ClientSession(connector=make_conn()) as session:
        async with session.get(
                f'https://localhost:{service_port}/hello',
        ) as response:
            assert response.status == 200
            assert await response.text() == 'Hello world!\n'
    # /// [Functional test]


async def test_no_cert(service_client, service_port):
    try:
        async with aiohttp.ClientSession(
                connector=make_conn(cert=False),
        ) as session:
            async with session.get(
                    f'https://localhost:{service_port}/hello',
            ) as _:
                assert False
    except aiohttp.client_exceptions.ClientOSError:
        pass


def make_conn_invalid():
    ssl_ctx = ssl.create_default_context(
        cafile=str(SERVICE_SOURCE_DIR / 'cert.crt'),
    )
    ssl_ctx.load_cert_chain(
        str(SERVICE_SOURCE_DIR / 'cert_invalid.crt'),
        str(SERVICE_SOURCE_DIR / 'private_invalid.key'),
    )

    conn = aiohttp.TCPConnector(ssl=ssl_ctx)
    return conn


async def test_invalid_cert(service_client, service_port):
    try:
        async with aiohttp.ClientSession(
                connector=make_conn_invalid(),
        ) as session:
            async with session.get(
                    f'https://localhost:{service_port}/hello',
            ) as _:
                assert False
    except aiohttp.client_exceptions.ClientOSError:
        pass
