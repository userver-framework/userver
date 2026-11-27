import asyncio
from signal import SIGTERM

import grpc
import pytest
import pytest_userver.utils.sync as sync

try:
    from grpc.health.v1 import health_pb2
except ImportError:
    from health.v1 import health_pb2


HEALTH_CHECK_TIMEOUT_SECONDS = 2.0


@pytest.mark.uservice_oneshot
async def test_graceful_shutdown_headers(
    service_daemon_instance,
    service_start_timeout,
    grpc_client,
    graceful_shutdown_headers,
    graceful_shutdown_first_stage_seconds,
):
    request = health_pb2.HealthCheckRequest()

    async def is_serving():
        response = await grpc_client.Check(request, timeout=HEALTH_CHECK_TIMEOUT_SECONDS)
        return response.status == health_pb2.HealthCheckResponse.SERVING

    await sync.wait(
        is_serving,
        catch=grpc.RpcError,
        total_wait_seconds=service_start_timeout,
        failure_msg='Service did not become SERVING before initiating graceful shutdown',
    )

    service_daemon_instance.process.send_signal(SIGTERM)

    async def wait_for_not_serving_with_headers():
        try:
            call = grpc_client.Check(request)
            response = await asyncio.wait_for(call, timeout=HEALTH_CHECK_TIMEOUT_SECONDS)
        except (grpc.RpcError, OSError, asyncio.TimeoutError):
            raise sync.NotReady()
        if response.status != health_pb2.HealthCheckResponse.NOT_SERVING:
            raise sync.NotReady()
        try:
            check_graceful_shutdown_headers(
                await call.initial_metadata(),
                await call.trailing_metadata(),
                graceful_shutdown_headers,
            )
        except AssertionError:
            raise sync.NotReady()

    # Health may flip to NOT_SERVING before graceful-shutdown headers middleware runs; keep
    # polling within the first shutdown stage (see conftest graceful_shutdown_first_stage_seconds).
    first_stage_reserve_seconds = 0.5
    await sync.wait_until(
        wait_for_not_serving_with_headers,
        relax_period_seconds=0.1,
        total_wait_seconds=graceful_shutdown_first_stage_seconds - first_stage_reserve_seconds,
    )

    service_daemon_instance.process.wait()


def check_graceful_shutdown_headers(initial, trailing, headers: dict[str, list[str]]) -> None:
    initial_dict = to_dict(initial)
    trailing_dict = to_dict(trailing)
    for key, values in headers.items():
        in_initial = key in initial_dict and initial_dict[key] == values
        in_trailing = key in trailing_dict and trailing_dict[key] == values
        assert in_initial != in_trailing, (
            f'header {key!r} must be present in exactly one of initial or trailing metadata, '
            f'got initial={initial_dict!r}, trailing={trailing_dict!r}'
        )


def to_dict(metadata) -> dict[str, list[str]]:
    result: dict[str, list[str]] = {}
    for k, v in metadata:
        result.setdefault(k, []).append(v)

    return result
