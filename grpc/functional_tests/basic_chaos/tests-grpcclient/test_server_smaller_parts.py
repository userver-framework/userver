import pytest

import requests_client

DATA_PARTS_MAX_SIZE = 40


@pytest.mark.parametrize('case', requests_client.ALL_CASES)
async def test_server_smaller_parts(grpc_ch, service_client, gate, case):
    gate.to_server_smaller_parts(DATA_PARTS_MAX_SIZE)
    await requests_client.check_200_for(case)(grpc_ch, service_client, gate)
