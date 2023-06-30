import pytest

import requests_server


@pytest.mark.parametrize('case', requests_server.ALL_CASES)
async def test_connection_reuse(grpc_client, gate, case):
    await requests_server.check_ok_for(case, grpc_client, gate)
    await gate.stop_accepting()
    await requests_server.check_ok_for(case, grpc_client, gate)
