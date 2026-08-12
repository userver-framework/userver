import asyncio
from signal import SIGTERM

import pytest

try:
    from grpc.health.v1 import health_pb2
except ImportError:
    from health.v1 import health_pb2


@pytest.mark.uservice_oneshot
async def test_graceful_shutdown_headers(service_daemon_instance, grpc_client, graceful_shutdown_headers):
    service_daemon_instance.process.send_signal(SIGTERM)
    await asyncio.sleep(1)  # Give the service time to process the signal.

    request = health_pb2.HealthCheckRequest()
    call = grpc_client.Check(request)
    response = await call
    assert response.status == health_pb2.HealthCheckResponse.NOT_SERVING

    check_present(await call.initial_metadata(), graceful_shutdown_headers)
    check_not_present(await call.trailing_metadata(), graceful_shutdown_headers)

    service_daemon_instance.process.wait()


def check_present(metadata, headers: dict[str, list[str]]):
    metadata_dict = to_dict(metadata)
    for k, v in headers.items():
        assert k in metadata_dict
        assert metadata_dict[k] == v


def check_not_present(metadata, headers: dict[str, list[str]]):
    metadata_dict = to_dict(metadata)
    for k in headers.keys():
        assert k not in metadata_dict


def to_dict(metadata) -> dict[str, list[str]]:
    result: dict[str, list[str]] = {}
    for k, v in metadata:
        result.setdefault(k, []).append(v)

    return result
