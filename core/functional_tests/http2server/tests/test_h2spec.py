# h2spec compliance tests https://github.com/summerwind/h2spec
#
# Names mirror h2spec spec IDs: <spec>/<section>/<case>.
# Section maps to a source file, e.g. http2/6_7_ping.go or generic/3_8_goaway.go.
# Case is the 1-based index of AddTestCase in that file.
import asyncio

import pytest

import utils


async def test_h2spec_6_7_2_ping_ack_ignored(create_connection, service_client):
    # http2/6_7_ping.go, case 2 ("Sends a PING frame with ACK"):
    # server must ignore PING frames with ACK flag set.
    await service_client.update_server_state()

    async with create_connection() as (sock, conn):
        ping_ack = utils.create_frame(utils.PING_FRAME, utils.ACK_FLAG, 0, b'\x00' * 8)
        await sock.sendall(ping_ack)

        with pytest.raises(asyncio.TimeoutError):
            await sock.recv(utils.RECEIVE_SIZE, timeout=0.5)


async def test_h2spec_generic_3_8_1_client_goaway(create_connection, monitor_client, service_client):
    # generic/3_8_goaway.go, case 1 ("Sends a GOAWAY frame"):
    # after client GOAWAY the server must close the connection or answer a follow-up PING.
    await service_client.update_server_state()

    async with monitor_client.metrics_diff(prefix='server.requests.http2') as differ:
        async with create_connection() as (sock, conn):
            conn.close_connection(error_code=0)
            ping_data = b'h2spec\x00\x00'
            ping = utils.create_frame(utils.PING_FRAME, utils.EMPTY_FLAGS, 0, ping_data)
            await sock.sendall(conn.data_to_send() + ping)

            receive = await sock.recv(utils.RECEIVE_SIZE, timeout=2.0)
            assert receive == b''

    assert differ.value_at('goaway') == 1
