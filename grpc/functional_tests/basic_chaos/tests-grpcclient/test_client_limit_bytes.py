import pytest

import requests_client

# Limits for server->client traffic. Unary say_hello sends a tiny response;
# with limits >= 100 the gRPC deadline (timeout=small, 1s) often wins before
# the byte-limit interceptor fires, so wait_call() flakes (USERVER-1309).
_CLIENT_LIMIT_BYTES = {
    'say_hello': range(10, 40, 10),
}
_DEFAULT_CLIENT_LIMIT_BYTES = range(100, 250, 50)


def _client_limit_bytes(case: str):
    return _CLIENT_LIMIT_BYTES.get(case, _DEFAULT_CLIENT_LIMIT_BYTES)


@pytest.mark.parametrize('case', requests_client.ALL_CASES)
async def test_client_limit_bytes(grpc_ch, service_client, gate, case):
    if case in ['say_hello_request_stream', 'say_hello_indept_streams']:
        pytest.skip(reason='Fails with "RPC cancelled for servicer method" then timeout on testsuite side')

    for i in _client_limit_bytes(case):
        queue = await gate.to_client_limit_bytes(i)
        await requests_client.unavailable_request(service_client, gate, case)
        await queue.wait_call(timeout=120)
    await requests_client.close_connection(gate, grpc_ch, service_client)
    await requests_client.check_200_for(case)(grpc_ch, service_client, gate)
