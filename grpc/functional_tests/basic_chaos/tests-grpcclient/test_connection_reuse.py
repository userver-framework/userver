import pytest

import requests_client

DATA_PARTS_MAX_SIZE = 40


@pytest.mark.parametrize('case', requests_client.ALL_CASES)
async def test_connection_reuse(grpc_ch, service_client, gate, case):
    await requests_client.check_200_for(case)(grpc_ch, service_client, gate)
    await gate.stop_accepting()
    await requests_client.check_200_for(case)(grpc_ch, service_client, gate)
