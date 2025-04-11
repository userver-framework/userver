import pytest

import requests_client


@pytest.mark.parametrize('case', requests_client.ALL_CASES)
async def test_server_limit_bytes(grpc_ch, service_client, gate, case):
    if case == 'say_hello_request_stream':
        # TODO(TAXICOMMON-9695) research the flaky test
        pytest.skip('Flaky')
        return

    for i in range(100, 250, 50):
        await gate.to_server_limit_bytes(i)
        await requests_client.unavailable_request(service_client, gate, case)

    await gate.to_server_pass()
    await requests_client.check_200_for(case)(grpc_ch, service_client, gate)
