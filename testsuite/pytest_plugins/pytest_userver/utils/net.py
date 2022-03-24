import asyncio


async def check_port_availability(hostname, port, timeout=1.0):
    try:
        _, writer = await asyncio.wait_for(
            asyncio.open_connection(hostname, port), timeout=timeout,
        )
    except (OSError, asyncio.TimeoutError):
        return False
    writer.close()
    await writer.wait_closed()
    return True
