import pytest

import requests_client


@pytest.mark.parametrize(
    'case', requests_client.ALL_CASES + ['request_without_case'],
)
async def test_basic(grpc_ch, service_client, gate, case):
    await requests_client.check_200_for(case)(grpc_ch, service_client, gate)
