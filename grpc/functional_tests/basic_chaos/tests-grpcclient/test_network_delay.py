import pytest

import requests_client

DATA_TRANSMISSION_DELAY = 2


@pytest.mark.parametrize('case', requests_client.ALL_CASES)
async def test_network_delay(grpc_ch, service_client, gate, case):
    gate.to_server_delay(DATA_TRANSMISSION_DELAY)
    gate.to_client_delay(DATA_TRANSMISSION_DELAY)
    # client with timeout 1 second
    await requests_client.unavailable_request(service_client, gate, case)
    gate.to_client_pass()
    gate.to_server_pass()
    await requests_client.check_200_for(case)(grpc_ch, service_client, gate)
