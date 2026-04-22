import asyncio
from signal import SIGTERM

import pytest

import samples.greeter_pb2 as greeter_protos

MD_ONE_REQ = ' One'
MD_TWO_REQ = ' Two'
MD_ONE_RES = ' EndOne'
MD_TWO_RES = ' EndTwo'


@pytest.mark.uservice_oneshot
async def test_graceful_shutdown_headers(service_daemon_instance, grpc_client, service_client, dynamic_config):
    params = [
        (True, {'x-envoy-immediate-health-check-fail': ['true']}),
        (False, {'x-envoy-immediate-health-check-fail': ['true']}),
        (True, {'x-hdr1': ['true', 'aaa'], 'x-hdr2': ['1', 'bbb', 'yyyy']}),
        (False, {'x-hdr1': ['true', 'aaa'], 'x-hdr2': ['1', 'bbb', 'yyyy']}),
    ]

    for headers_enabled, headers in params:
        await update_graceful_shutdown_headers(service_client, dynamic_config, headers_enabled, headers)

        request = greeter_protos.GreetingRequest(name='Python')
        call = grpc_client.SayHello(request)
        response = await call
        assert response.greeting == f'Hello, Python{MD_ONE_REQ}{MD_TWO_REQ}!{MD_TWO_RES}{MD_ONE_RES}'
        check_not_present(await call.initial_metadata(), headers)
        check_not_present(await call.trailing_metadata(), headers)

    service_daemon_instance.process.send_signal(SIGTERM)

    for headers_enabled, headers in params:
        await update_graceful_shutdown_headers(service_client, dynamic_config, headers_enabled, headers)

        request = greeter_protos.GreetingRequest(name='Python')
        call = grpc_client.SayHello(request)
        response = await call
        assert response.greeting == f'Hello, Python{MD_ONE_REQ}{MD_TWO_REQ}!{MD_TWO_RES}{MD_ONE_RES}'

        if headers_enabled:
            check_present(await call.initial_metadata(), headers)
        else:
            check_not_present(await call.initial_metadata(), headers)

        check_not_present(await call.trailing_metadata(), headers)

    # After a couple more seconds, the service will start shutting down.
    service_daemon_instance.process.wait()


@pytest.mark.uservice_oneshot
async def test_graceful_shutdown_headers_streams(service_daemon_instance, grpc_client, service_client, dynamic_config):
    headers = {'x-envoy-immediate-health-check-fail': ['true']}
    await update_graceful_shutdown_headers(service_client, dynamic_config, True, headers)

    start = f'Python{MD_ONE_REQ}{MD_TWO_REQ}'
    end = f'{MD_TWO_RES}{MD_ONE_RES}'

    service_daemon_instance.process.send_signal(SIGTERM)

    call = grpc_client.SayHelloStreams(
        _prepare_requests(['Python', '!', '!', '!'], 0.1),
        wait_for_ready=True,
    )
    async for response in call:
        assert response.greeting == f'Hello, {start}{end}'
        start += f'!{MD_ONE_REQ}{MD_TWO_REQ}'

    check_present(await call.initial_metadata(), headers)
    check_not_present(await call.trailing_metadata(), headers)

    # After a couple more seconds, the service will start shutting down.
    service_daemon_instance.process.wait()


@pytest.mark.uservice_oneshot
async def test_late_graceful_shutdown_headers_streams(
    service_daemon_instance, grpc_client, service_client, dynamic_config
):
    headers = {'x-envoy-immediate-health-check-fail': ['true']}
    await update_graceful_shutdown_headers(service_client, dynamic_config, True, headers)

    start = f'Python{MD_ONE_REQ}{MD_TWO_REQ}'
    end = f'{MD_TWO_RES}{MD_ONE_RES}'

    call = grpc_client.SayHelloStreams(
        _prepare_late_requests(service_daemon_instance, ['Python', '!', '!'], 0.2),
        wait_for_ready=True,
    )
    async for response in call:
        assert response.greeting == f'Hello, {start}{end}'
        start += f'!{MD_ONE_REQ}{MD_TWO_REQ}'

    check_not_present(await call.initial_metadata(), headers)
    check_present(await call.trailing_metadata(), headers)

    # After a couple more seconds, the service will start shutting down.
    service_daemon_instance.process.wait()


async def _prepare_requests(names, sleep=1):
    reqs = []
    for name in names:
        reqs.append(greeter_protos.GreetingRequest(name=name))
    for req in reqs:
        await asyncio.sleep(sleep)
        yield req


async def _prepare_late_requests(service_daemon_instance, names, sleep=1):
    reqs = []
    for name in names:
        reqs.append(greeter_protos.GreetingRequest(name=name))
    i = 0
    for req in reqs:
        if i == 1:
            service_daemon_instance.process.send_signal(SIGTERM)
        i += 1
        await asyncio.sleep(sleep)
        yield req


async def update_graceful_shutdown_headers(service_client, dynamic_config, headers_enabled, headers):
    dynamic_config.set_values({'GRACEFUL_SHUTDOWN_HEADERS': {'enabled': headers_enabled, 'headers': headers}})
    await service_client.update_server_state()


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
