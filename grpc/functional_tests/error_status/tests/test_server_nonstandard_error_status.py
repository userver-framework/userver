import functional_tests.error_status_pb2 as error_status_protos
import pytest

# gRPC status codes
GRPC_STATUS_OK = 0
GRPC_STATUS_UNKNOWN = 2

# Service name from proto definition
SERVICE_NAME = 'functional_tests.api.ErrorStatusService'

INVALID_STATUSES = ['17', '100', '255', '1000', '100000']
EXPECTED_MESSAGE = 'Test invalid status'


@pytest.fixture
async def supports_non_standard_status(raw_grpc_client):
    request = error_status_protos.NonStandardErrorStatusSupportRequest()
    response = await raw_grpc_client.call(
        SERVICE_NAME,
        'SupportsNonStandardStatus',
        request.SerializeToString(),
    )

    assert response.status == GRPC_STATUS_OK

    result = error_status_protos.NonStandardErrorStatusSupportResponse()
    result.ParseFromString(response.data)
    return result.supports_non_standard_statuses


@pytest.mark.parametrize('invalid_status_code', INVALID_STATUSES)
async def test_server_converts_invalid_status_to_unknown(
    raw_grpc_client,
    supports_non_standard_status: bool,
    invalid_status_code: str,
):
    if not supports_non_standard_status:
        pytest.skip('UBSan enabled')

    request = error_status_protos.ErrorStatusRequest(
        status_code=invalid_status_code,
        status_message=EXPECTED_MESSAGE,
    )

    response = await raw_grpc_client.call(
        SERVICE_NAME,
        'ReturnNonStandardErrorStatus',
        request.SerializeToString(),
    )

    # The client should receive UNKNOWN status for invalid codes
    assert response.status == GRPC_STATUS_UNKNOWN, (
        f'Expected UNKNOWN (2), got {response.status}: {response.message}. userver must clamp out-of-range statuses to UNKNOWN'
    )
    assert response.message == EXPECTED_MESSAGE, f'Expected message {EXPECTED_MESSAGE}, got {response.message}'


@pytest.mark.parametrize('invalid_status_code', INVALID_STATUSES)
async def test_server_stream_converts_invalid_status_to_unknown(
    raw_grpc_client,
    supports_non_standard_status: bool,
    invalid_status_code: str,
):
    if not supports_non_standard_status:
        pytest.skip('UBSan enabled')

    request = error_status_protos.ErrorStatusRequest(
        status_code=invalid_status_code,
        status_message=EXPECTED_MESSAGE,
    )

    response = await raw_grpc_client.call(
        SERVICE_NAME,
        'ReturnStreamNonStandardErrorStatus',
        request.SerializeToString(),
    )

    # Should have received some responses before the error
    # (response.data contains concatenated protobuf messages from all frames)
    assert len(response.data) > 0, 'Expected some stream responses before error'

    # The client should receive UNKNOWN status for invalid codes
    assert response.status == GRPC_STATUS_UNKNOWN, (
        f'Expected UNKNOWN (2), got {response.status}: {response.message}. userver must clamp out-of-range statuses to UNKNOWN'
    )
    assert response.message == EXPECTED_MESSAGE, f'Expected message {EXPECTED_MESSAGE}, got {response.message}'
