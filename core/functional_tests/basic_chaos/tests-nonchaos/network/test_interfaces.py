import asyncio

import netifaces
import pytest


def get_nonlocal_ips():
    result = []
    for interface in netifaces.interfaces():
        if interface.startswith('lo'):
            continue

        info = netifaces.ifaddresses(interface)
        result.extend([x['addr'] for x in info.get(netifaces.AF_INET6, [])])

    return result


@pytest.mark.skip(reason='fails locally')
async def test_closed_from_external(service_client, service_port):
    for addr in get_nonlocal_ips():
        try:
            _, _ = await asyncio.open_connection(addr, service_port)
            assert False, (
                'Managed to connect through forbidden '
                f'external address: {addr, service_port}'
            )
        except (OSError, asyncio.TimeoutError):
            pass
