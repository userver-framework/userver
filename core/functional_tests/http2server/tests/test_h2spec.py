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
