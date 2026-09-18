import asyncio
from signal import SIGTERM

import grpc
import pytest
import pytest_userver.utils.sync as sync

try:
    from grpc.health.v1 import health_pb2
except ImportError:
    from health.v1 import health_pb2


@pytest.mark.uservice_oneshot
async def test_graceful_shutdown_headers(service_daemon_instance, grpc_client, graceful_shutdown_headers):
    service_daemon_instance.process.send_signal(SIGTERM)

    request = health_pb2.HealthCheckRequest()

    async def wait_for_not_serving():
        try:
            call = grpc_client.Check(request)
            response = await asyncio.wait_for(call, timeout=1.0)
        except (grpc.RpcError, OSError, asyncio.TimeoutError):
            raise sync.NotReady()
        if response.status != health_pb2.HealthCheckResponse.NOT_SERVING:
            raise sync.NotReady()
        return call

    call = await sync.wait_until(
        wait_for_not_serving,
        relax_period_seconds=0.1,
        total_wait_seconds=5.0,
    )

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
