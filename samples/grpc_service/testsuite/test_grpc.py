import sys

import pytest

# /// [Prepare modules]
import samples.greeter_pb2 as greeter_protos
import samples.greeter_pb2_grpc as greeter_services

# /// [Prepare modules]


# /// [grpc client test]
async def test_grpc_client_mock_say_hello(service_client, grpc_mockserver):
    @grpc_mockserver(greeter_services.GreeterServiceServicer.SayHello)
    async def mock_say_hello(request, context):
        return greeter_protos.GreetingResponse(
            greeting=f'Hello, {request.name} from mockserver!',
        )

    response = await service_client.post('/hello?case=say_hello', data='tests')
    assert response.status == 200
    assert 'text/plain' in response.headers['Content-Type']
    assert response.text == 'Hello, tests from mockserver!'
    assert mock_say_hello.times_called == 1
    # /// [grpc client test]


# /// [grpc client test response stream]
async def test_grpc_client_mock_say_hello_response_stream(
    service_client,
    grpc_mockserver,
):
    @grpc_mockserver(greeter_services.GreeterServiceServicer.SayHelloResponseStream)
    async def mock_say_hello_response_stream(request, context):
        message = f'Hello, {request.name}'
        for i in range(5):
            message += '!'
            yield greeter_protos.GreetingResponse(greeting=message)

    response = await service_client.post(
        '/hello?case=say_hello_response_stream',
        data='Python',
    )
    assert response.status == 200
    assert 'text/plain' in response.headers['Content-Type']
    assert (
        response.text
        == """Hello, Python!
Hello, Python!!
Hello, Python!!!
Hello, Python!!!!
Hello, Python!!!!!
"""
    )
    assert mock_say_hello_response_stream.times_called == 1
    # /// [grpc client test response stream]


# /// [grpc client test request stream]
async def test_grpc_client_mock_say_hello_request_stream(
    service_client,
    grpc_mockserver,
):
    @grpc_mockserver(greeter_services.GreeterServiceServicer.SayHelloRequestStream)
    async def mock_say_hello_request_stream(request_iterator, context):
        message = 'Hello, '
        async for request in request_iterator:
            message += f'{request.name}'
        return greeter_protos.GreetingResponse(greeting=message)

    response = await service_client.post(
        '/hello?case=say_hello_request_stream',
        data='Python\n!\n!\n!',
    )
    assert response.status == 200
    assert 'text/plain' in response.headers['Content-Type']
    assert response.text == 'Hello, Python!!!'
    assert mock_say_hello_request_stream.times_called == 1
    # /// [grpc client test request stream]


# /// [grpc client test streams]
async def test_grpc_client_mock_say_hello_streams(
    service_client,
    grpc_mockserver,
):
    @grpc_mockserver(greeter_services.GreeterServiceServicer.SayHelloStreams)
    async def mock_say_hello_streams(request_iterator, context):
        message = 'Hello, '
        async for request in request_iterator:
            message += f'{request.name}'
            yield greeter_protos.GreetingResponse(greeting=message)

    response = await service_client.post(
        '/hello?case=say_hello_streams',
        data='Python\n!\n!\n!',
    )
    assert response.status == 200
    assert 'text/plain' in response.headers['Content-Type']
    assert (
        response.text
        == """Hello, Python
Hello, Python!
Hello, Python!!
Hello, Python!!!
"""
    )
    assert mock_say_hello_streams.times_called == 1
    # /// [grpc client test streams]


@pytest.mark.skipif(
    sys.platform == 'darwin',
    reason='this test fails in old packages',
)
# /// [grpc server test]
async def test_say_hello(grpc_client):
    request = greeter_protos.GreetingRequest(name='Python')
    response = await grpc_client.SayHello(request)
    assert response.greeting == 'Hello, Python!'
    # /// [grpc server test]


# /// [grpc server test response stream]
async def test_say_hello_response_stream(grpc_client):
    request = greeter_protos.GreetingRequest(name='Python')
    message = 'Hello, Python'
    async for response in grpc_client.SayHelloResponseStream(request):
        message += '!'
        assert response.greeting == message
        # /// [grpc server test response stream]


# /// [grpc server test request stream]
async def test_say_hello_request_stream(grpc_client):
    request = (greeter_protos.GreetingRequest(name=name) for name in ['Python', '!', '!', '!'])
    response = await grpc_client.SayHelloRequestStream(request)
    assert response.greeting == 'Hello, Python!!!'
    # /// [grpc server test request stream]


# /// [grpc server test streams]
async def test_say_hello_streams(grpc_client):
    requests = (greeter_protos.GreetingRequest(name=name) for name in ['Python', '!', '!', '!'])
    message = 'Hello, Python'
    async for response in grpc_client.SayHelloStreams(requests):
        assert response.greeting == message
        message += '!'
        # /// [grpc server test streams]
