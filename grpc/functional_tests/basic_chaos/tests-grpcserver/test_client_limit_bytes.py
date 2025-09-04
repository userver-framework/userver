import pytest
import requests_server


@pytest.mark.parametrize('case', requests_server.ALL_CASES)
async def test_client_limit_bytes(grpc_client, gate, case):
    for i in (50, 100, 150):
        queue = await gate.to_client_limit_bytes(i)
        await requests_server.check_unavailable_for(case, grpc_client, gate)
        await queue.wait_call()

    await gate.to_client_pass()
    await requests_server.check_ok_for(case, grpc_client, gate)
