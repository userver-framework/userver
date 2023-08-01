import pytest

import requests_client


@pytest.mark.parametrize('case', requests_client.ALL_CASES)
async def test_close_clonnection(grpc_ch, service_client, gate, case):
    if case == 'say_hello_streams':
        pytest.skip('Flaky TAXICOMMON-7319')
    await requests_client.close_connection(gate, grpc_ch)
    await requests_client.check_200_for(case)(grpc_ch, service_client, gate)
