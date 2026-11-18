import asyncio
import logging

import grpc
import pytest

import samples.greeter_pb2 as greeter_pb2

logger = logging.getLogger(__name__)

ALL_CASES = [
    'say_hello',
    'say_hello_response_stream',
    'say_hello_request_stream',
    'say_hello_streams',
    'say_hello_indept_streams',
]

OK_RETRIES_COUNT = 10
RPC_TIMEOUT_SECONDS = 10.0
RPC_COMPLETION_TIMEOUT_SECONDS = 15.0
# A complete request/response must not fit within any tested byte limit.
FAULT_PADDING = 'x' * 1024


async def _say_hello(grpc_client, gate, *, padding=''):
    name = f'Python{padding}'
    request = greeter_pb2.GreetingRequest(name=name)
    response = await grpc_client.SayHello(request, timeout=RPC_TIMEOUT_SECONDS, wait_for_ready=True)
    assert response.greeting == f'Hello, {name}!'
    assert gate.connections_count() > 0


async def _say_hello_response_stream(grpc_client, gate, *, padding=''):
    name = f'Python{padding}'
    request = greeter_pb2.GreetingRequest(name=name)
    reference = '!'
    async for response in grpc_client.SayHelloResponseStream(
        request,
        timeout=RPC_TIMEOUT_SECONDS,
        wait_for_ready=True,
    ):
        assert response.greeting == f'Hello, {name}{reference}'
        reference += '!'
    assert gate.connections_count() > 0


async def _prepare_requests(names, sleep=1):
    reqs = []
    for name in names:
        reqs.append(greeter_pb2.GreetingRequest(name=name))
    for req in reqs:
        await asyncio.sleep(sleep)
        yield req


async def _say_hello_request_stream(grpc_client, gate, *, padding=''):
    name = f'Python{padding}'
    stream = await grpc_client.SayHelloRequestStream(
        _prepare_requests([name, '!', '!', '!'], 1),
        timeout=RPC_TIMEOUT_SECONDS,
        wait_for_ready=True,
    )
    assert stream.greeting == f'Hello, {name}!!!'
    assert gate.connections_count() > 0


async def _say_hello_streams(grpc_client, gate, *, padding=''):
    name = f'Python{padding}'
    reference = ''
    async for response in grpc_client.SayHelloStreams(
        _prepare_requests([name, '!', '!', '!'], 1),
        timeout=RPC_TIMEOUT_SECONDS,
        wait_for_ready=True,
    ):
        assert response.greeting == f'Hello, {name}{reference}'
        reference += '!'
    assert gate.connections_count() > 0


async def _say_hello_indept_streams(grpc_client, gate, *, padding=''):
    expected_responses = [
        'Hello, Python',
        'Hello, C++',
        'Hello, linux',
        'Hello, userver',
        'Hello, grpc',
        'Hello, kernel',
        'Hello, developer',
        'Hello, core',
        'Hello, anonymous',
        'Hello, user',
        f'If{padding} this message has arrived, then everything works',
    ]

    index = 0
    async for response in grpc_client.SayHelloIndependentStreams(
        _prepare_requests(
            [
                f'If{padding}',
                ' ',
                'this',
                ' ',
                'message',
                ' ',
                'has',
                ' ',
                'arrived,',
                ' ',
                'then',
                ' ',
                'everything',
                ' ',
                'works',
            ],
            0.1,
        ),
        timeout=RPC_TIMEOUT_SECONDS,
        wait_for_ready=True,
    ):
        assert response.greeting == expected_responses[index]
        index += 1
    assert gate.connections_count() > 0


_REQUESTS = {
    'say_hello': _say_hello,
    'say_hello_response_stream': _say_hello_response_stream,
    'say_hello_request_stream': _say_hello_request_stream,
    'say_hello_streams': _say_hello_streams,
    'say_hello_indept_streams': _say_hello_indept_streams,
}


async def check_ok_for(case, grpc_client, gate):
    for i in range(OK_RETRIES_COUNT):
        try:
            await asyncio.wait_for(
                _REQUESTS[case](grpc_client, gate),
                timeout=RPC_COMPLETION_TIMEOUT_SECONDS,
            )
            logging.info(f'OK at "{case}" (attempt {i})')
            return
        except grpc.RpcError as error:
            logger.warning(f'Error at "{case}" (attempt {i}): {error.code()}')
        except asyncio.TimeoutError:
            logger.warning(f'Error at "{case}" (attempt {i}): timeout')
    assert False, f'{case}: all attempts failed'


async def prepare_for_fault(grpc_client, gate):
    await close_connection(gate)
    await check_ok_for('say_hello', grpc_client, gate)
    gate.sockets_closed_event.clear()


async def check_failure_for(case, grpc_client, gate):
    with pytest.raises(grpc.RpcError) as exc_info:
        await asyncio.wait_for(
            _REQUESTS[case](grpc_client, gate, padding=FAULT_PADDING),
            timeout=RPC_COMPLETION_TIMEOUT_SECONDS,
        )
    # Reconnects can hit the byte limit during the HTTP/2 handshake, so
    # wait_for_ready may keep the RPC queued until its deadline.
    assert exc_info.value.code() in (grpc.StatusCode.UNAVAILABLE, grpc.StatusCode.DEADLINE_EXCEEDED)
    assert gate.sockets_closed_event.is_set(), 'Byte limit did not close any connections'


async def check_recovery_for(case, grpc_client, gate):
    await close_connection(gate)
    await check_ok_for(case, grpc_client, gate)


async def close_connection(gate):
    await gate.to_server_pass()
    await gate.to_client_pass()
    gate.start_accepting()
    await gate.sockets_close()
