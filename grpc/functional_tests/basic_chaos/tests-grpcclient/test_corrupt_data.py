import pytest

import requests_client


@pytest.mark.parametrize('case', requests_client.ALL_CASES)
async def test_corrupt_data(grpc_ch, service_client, gate, case):
    gate.to_client_corrupt_data()

    for _ in range(gate.connections_count()):
        await requests_client.unavailable_request(service_client, gate, case)

    await requests_client.close_connection(gate, grpc_ch)
    await requests_client.check_200_for(case)(grpc_ch, service_client, gate)
