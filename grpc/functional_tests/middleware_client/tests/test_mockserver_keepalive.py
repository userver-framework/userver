import asyncio


async def test_mockserver_accepts_keepalive_pings(grpc_mockserver, grpc_mockserver_endpoint):
    host, _, port = grpc_mockserver_endpoint.rpartition(':')
    reader, writer = await asyncio.open_connection(host.strip('[]'), int(port))
    try:
        # Start an HTTP/2 connection with an empty SETTINGS frame and no RPCs.
        writer.write(b'PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n' + b'\x00\x00\x00\x04\x00\x00\x00\x00\x00')
        await writer.drain()

        # Exceed the default ping-strike limit without sending any data frames.
        for ping_id in range(10):
            payload = ping_id.to_bytes(8, 'big')
            writer.write(b'\x00\x00\x08\x06\x00\x00\x00\x00\x00' + payload)
            await writer.drain()
            while True:
                header = await reader.readexactly(9)
                frame_payload = await reader.readexactly(int.from_bytes(header[:3], 'big'))
                frame_type, flags = header[3:5]
                assert frame_type != 7, f'Unexpected GOAWAY: {frame_payload!r}'
                if frame_type == 4 and flags == 0:
                    # Acknowledge the server's SETTINGS.
                    writer.write(b'\x00\x00\x00\x04\x01\x00\x00\x00\x00')
                    await writer.drain()
                elif frame_type == 6 and flags == 1:
                    assert frame_payload == payload
                    break
    finally:
        writer.close()
        await writer.wait_closed()
