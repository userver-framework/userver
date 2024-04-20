import samples.greeter_pb2 as greeter_protos  # noqa: E402, E501


async def test_say_hello(grpc_client):
    request = greeter_protos.GreetingRequest(name='Python')
    response = await grpc_client.SayHello(request)
    assert response.greeting == 'Hello, Python!'
