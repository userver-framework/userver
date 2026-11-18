import pytest
import requests_server


@pytest.mark.parametrize('case', requests_server.ALL_CASES)
async def test_network_limit_bytes(grpc_client, gate, case):
    for i in (50, 100, 150):
        await requests_server.prepare_for_fault(grpc_client, gate)
        await gate.to_client_limit_bytes(i)
        await gate.to_server_limit_bytes(i)
        await requests_server.check_failure_for(case, grpc_client, gate)
        await requests_server.check_recovery_for(case, grpc_client, gate)
