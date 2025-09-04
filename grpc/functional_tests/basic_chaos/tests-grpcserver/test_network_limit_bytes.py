import pytest
import requests_server


@pytest.mark.parametrize('case', requests_server.ALL_CASES)
async def test_network_limit_bytes(grpc_client, gate, case):
    for i in (50, 100, 150):
        await gate.to_client_limit_bytes(i)
        await gate.to_server_limit_bytes(i)
        await requests_server.check_unavailable_for(case, grpc_client, gate)

    await gate.to_client_pass()
    await gate.to_server_pass()
    await requests_server.check_ok_for(case, grpc_client, gate)
