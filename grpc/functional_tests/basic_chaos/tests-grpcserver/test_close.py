import pytest

import requests_server


@pytest.mark.parametrize('case', requests_server.ALL_CASES)
async def test_close(grpc_client, gate, case):
    await requests_server.close_connection(gate)
    await requests_server.check_ok_for(case, grpc_client, gate)
