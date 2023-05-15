import asyncio

import samples.greeter_pb2 as greeter_pb2  # noqa: E402, E501


async def test_say_hello(grpc_client, gate):
    request = greeter_pb2.GreetingRequest(name='Python')
    response = await grpc_client.SayHello(request)
    assert response.greeting == 'Hello, Python!'
    assert gate.connections_count() > 0


async def test_say_hello_response_stream(grpc_client, gate):
    request = greeter_pb2.GreetingRequest(name='Python')
    reference = '!'
    async for response in grpc_client.SayHelloResponseStream(request):
        assert response.greeting == f'Hello, Python{reference}'
        reference += '!'
    assert gate.connections_count() > 0


async def _prepare_requests(names, sleep=1):
    reqs = []
    for name in names:
        reqs.append(greeter_pb2.GreetingRequest(name=name))
    for req in reqs:
        await asyncio.sleep(sleep)
        yield req


async def test_say_hello_request_stream(grpc_client, gate):

    stream = await grpc_client.SayHelloRequestStream(
        _prepare_requests(['Python', '!', '!', '!'], 1),
    )
    assert stream.greeting == 'Hello, Python!!!'
    assert gate.connections_count() > 0


async def test_say_hello_streams(grpc_client, gate):
    reference = ''
    async for response in grpc_client.SayHelloStreams(
            _prepare_requests(['Python', '!', '!', '!'], 1),
    ):
        assert response.greeting == f'Hello, Python{reference}'
        reference += '!'
    assert gate.connections_count() > 0


async def test_say_hello_indept_streams(grpc_client, gate):
    expend_responses = [
        'Hello, Python',
        'Hello, C++',
        'Hello, linux',
        'Hello, userver',
        'Hello, grpc',
        'Hello, kernel',
        'Hello, developer',
        'Hello, core',
        'Hello, anonim',
        'Hello, user',
        'If this message has arrived, then everything works',
    ]

    index = 0
    async for response in grpc_client.SayHelloIndependentStreams(
            _prepare_requests(
                [
                    'If',
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
    ):
        assert response.greeting == expend_responses[index]
        index += 1
    assert gate.connections_count() > 0
