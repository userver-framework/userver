import pytest

import requests_server

DATA_PARTS_MAX_SIZE = 40


@pytest.mark.parametrize('case', requests_server.ALL_CASES)
async def test_client_smaller_parts(grpc_client, gate, case):
    gate.to_client_smaller_parts(DATA_PARTS_MAX_SIZE)
    await requests_server.check_ok_for(case, grpc_client, gate)
