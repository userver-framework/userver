import asyncio

import pytest

CLOSE_WAIT_TIMEOUT_SECONDS = 30

INCOMPLETE_CLIENT_HELLO = b'\x16\x03\x01\x02\x00'


@pytest.mark.parametrize(
    'client_data',
    [
        pytest.param(b'', id='silent'),
        pytest.param(INCOMPLETE_CLIENT_HELLO, id='incomplete_client_hello'),
    ],
)
async def test_stalled_handshake_is_dropped(service_client, service_port, client_data):
    reader, writer = await asyncio.open_connection('localhost', service_port)
    try:
        if client_data:
            writer.write(client_data)
            await writer.drain()

        try:
            await asyncio.wait_for(reader.read(), timeout=CLOSE_WAIT_TIMEOUT_SECONDS)
        except ConnectionResetError:
            pass
    finally:
        writer.close()

    response = await service_client.get('/ping')
    assert response.status == 200
