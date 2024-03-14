import asyncio
import pathlib

import pytest

SERVICE_SOURCE_DIR = pathlib.Path(__file__).parent.parent


async def test_cancellable(service_client, testpoint, service_port):
    @testpoint('testpoint_cancel')
    def cancel_testpoint(data):
        pass

    less_than_handler_sleep_time = 1.0
    with pytest.raises(asyncio.TimeoutError):
        await service_client.get(
            '/https/httpserver',
            params={'type': 'cancel'},
            timeout=less_than_handler_sleep_time,
        )
    await cancel_testpoint.wait_call()
