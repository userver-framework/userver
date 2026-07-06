# h2spec compliance tests https://github.com/summerwind/h2spec/tree/master/http2
#
# Names mirror h2spec spec IDs: http2/<section>/<case>.
# Section maps to a source file, e.g. http2/6_7_ping.go -> RFC 7540 §6.7 (PING).
# Case is the 1-based index of AddTestCase in that file; run via `h2spec http2/6.7/2`.
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
