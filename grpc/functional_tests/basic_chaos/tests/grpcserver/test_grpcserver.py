import time


async def test_say_hello(greeter_protos, grpc_client, gate):
    request = greeter_protos.GreetingRequest(name='Python')
    response = await grpc_client.SayHello(request)
    assert response.greeting == 'Hello, Python!'
    assert gate.connections_count() > 0


async def test_say_hello_response_stream(greeter_protos, grpc_client, gate):
    request = greeter_protos.GreetingRequest(name='Python')
    reference = '!'
    async for response in grpc_client.SayHelloResponseStream(request):
        assert response.greeting == f'Hello, Python{reference}'
        reference += '!'
    assert gate.connections_count() > 0


def _prepare_requests(greeter_protos, names, sleep=1):
    reqs = []
    for name in names:
        reqs.append(greeter_protos.GreetingRequest(name=name))
    for req in reqs:
        yield req
        time.sleep(sleep)


async def test_say_hello_request_stream(greeter_protos, grpc_client, gate):

    stream = await grpc_client.SayHelloRequestStream(
        _prepare_requests(greeter_protos, ['Python', '!', '!', '!'], 1),
    )
    assert stream.greeting == 'Hello, Python!!!'
    assert gate.connections_count() > 0


async def test_say_hello_streams(greeter_protos, grpc_client, gate):
    reference = ''
    async for response in grpc_client.SayHelloStreams(
            _prepare_requests(greeter_protos, ['Python', '!', '!', '!'], 1),
    ):
        assert response.greeting == f'Hello, Python{reference}'
        reference += '!'
    assert gate.connections_count() > 0


async def test_say_hello_indept_streams(greeter_protos, grpc_client, gate):
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
                greeter_protos,
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
