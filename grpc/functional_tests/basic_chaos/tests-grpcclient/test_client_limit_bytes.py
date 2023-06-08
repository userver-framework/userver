import pytest

import requests_client


@pytest.mark.parametrize('case', requests_client.ALL_CASES)
async def test_client_limit_bytes(grpc_ch, service_client, gate, case):
    for i in range(100, 250, 50):
        gate.to_client_limit_bytes(i)
        await requests_client.unavailable_request(service_client, gate, case)
    await requests_client.close_connection(gate, grpc_ch)
    await requests_client.check_200_for(case)(grpc_ch, service_client, gate)
