import pytest

import requests_server


@pytest.mark.skip(reason='This test flaps(')
@pytest.mark.parametrize('case', requests_server.CASES_WITHOUT_INDEPT_STREAMS)
async def test_server_limit_bytes(grpc_client, gate, case):
    for i in range(10, 100, 25):
        gate.to_server_limit_bytes(i)
        await requests_server.check_unavailable_for(case, grpc_client, gate)

    await requests_server.close_connection(gate)
    await requests_server.check_ok_for(case, grpc_client, gate)
