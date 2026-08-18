"""Websockets bootstrapped over HTTP/2.0 with the extended CONNECT of RFC 8441.

The service is the very same one the HTTP/1.1 websocket test drives; only the
listener speaks HTTP/2.0 with `enable_connect_protocol` on.
"""

import pytest
import wsproto.events

from conftest import Rfc8441Error


async def test_setting_is_advertised(rfc8441_client):
    with rfc8441_client() as client:
        assert client.enable_connect_protocol == 1


async def test_echo(rfc8441_client):
    with rfc8441_client() as client:
        assert client.connect('/chat') == '200'
        client.send_text('hello')
        assert client.recv_message() == 'hello'
        client.send_text('second message')
        assert client.recv_message() == 'second message'


async def test_echo_bin(rfc8441_client):
    with rfc8441_client() as client:
        assert client.connect('/chat') == '200'
        client.send_bytes(b'\x00\x01\x02\xff')
        assert client.recv_message() == b'\x00\x01\x02\xff'


async def test_handshake_hook_sees_the_request(rfc8441_client):
    with rfc8441_client() as client:
        # The handler echoes the Origin back as its first message, which proves
        # HandleHandshake() ran and saw the real headers.
        assert client.connect('/chat', extra_headers=[('origin', 'localhost')]) == '200'
        assert client.recv_message() == 'localhost'


async def test_close_handshake(rfc8441_client):
    with rfc8441_client() as client:
        assert client.connect('/chat') == '200'
        client.send_text('hello')
        assert client.recv_message() == 'hello'
        assert isinstance(client.close(), wsproto.events.CloseConnection)


async def test_server_initiated_close(rfc8441_client):
    with rfc8441_client() as client:
        assert client.connect('/chat') == '200'
        client.send_text('close')
        closed = client.recv()
        assert isinstance(closed, wsproto.events.CloseConnection)
        assert closed.code == 1001


async def test_multiplexing_with_a_plain_request(rfc8441_client, service_client):
    """The property the HTTP/1.1 transport can never have."""
    with rfc8441_client() as client:
        assert client.connect('/chat') == '200'
        client.send_text('before')
        assert client.recv_message() == 'before'

        # Another stream of the *same* connection, while the websocket is open.
        assert client.status_of(client.request('/ping')) == '404'

        client.send_text('after')
        assert client.recv_message() == 'after'


async def test_two_websockets_on_one_connection(rfc8441_client):
    with rfc8441_client() as first, rfc8441_client() as second:
        assert first.connect('/chat') == '200'
        assert second.connect('/handler-alt') == '200'

        first.send_text('to first')
        second.send_text('to second')
        assert first.recv_message() == 'to first'
        assert second.recv_message() == 'to second'


async def test_unknown_path_is_not_upgraded(rfc8441_client):
    with rfc8441_client() as client:
        assert client.connect('/no-such-handler') == '404'


async def test_non_websocket_handler_does_not_answer_2xx(rfc8441_client):
    """A 2xx to an extended CONNECT would tell the client the tunnel is up."""
    with rfc8441_client() as client:
        assert client.connect('/plain') == '502'


async def test_non_websocket_protocol_is_rejected(rfc8441_client):
    with rfc8441_client() as client:
        client._stream_id = client._conn.get_next_available_stream_id()  # noqa: SLF001
        client._conn.send_headers(  # noqa: SLF001
            client._stream_id,  # noqa: SLF001
            [
                (':method', 'CONNECT'),
                (':protocol', 'mqtt'),
                (':scheme', 'http'),
                (':path', '/chat'),
                (':authority', client._authority),  # noqa: SLF001
            ],
            end_stream=False,
        )
        client._flush()  # noqa: SLF001
        with pytest.raises(Rfc8441Error):
            client.recv()


async def test_service_survives_an_abandoned_websocket(rfc8441_client, service_client):
    with rfc8441_client() as client:
        assert client.connect('/chat') == '200'
        client.send_text('hello')
        assert client.recv_message() == 'hello'
        client.disconnect()

    response = await service_client.get('/ping')
    assert response.status in (404, 200)
