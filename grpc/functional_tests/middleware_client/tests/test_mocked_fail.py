import pytest_userver.grpc

import samples.greeter_pb2 as greeter_protos
import samples.greeter_pb2_grpc as greeter_services


# /// [Mocked network failure]
async def test_network_fail(grpc_client, grpc_mockserver_new):
    @grpc_mockserver_new(greeter_services.GreeterServiceServicer.SayHello)
    async def mock_say_hello(request, context):
        raise pytest_userver.grpc.NetworkError()

    request = greeter_protos.GreetingRequest(name='No matter')
    response = await grpc_client.SayHello(request)
    assert (
        response.greeting
        == "Client caught mocked error: 'samples.api.GreeterService/SayHello' failed: interrupted at Testsuite network"
    )
    # /// [Mocked network failure]


# /// [Mocked timeout failure]
async def test_timeout_fail(grpc_client, grpc_mockserver_new):
    @grpc_mockserver_new(greeter_services.GreeterServiceServicer.SayHello)
    async def mock_say_hello(request, context):
        raise pytest_userver.grpc.TimeoutError()

    request = greeter_protos.GreetingRequest(name='No matter')
    response = await grpc_client.SayHello(request)
    assert (
        response.greeting
        == "Client caught mocked error: 'samples.api.GreeterService/SayHello' failed: interrupted at Testsuite timeout"
    )
    # /// [Mocked timeout failure]
