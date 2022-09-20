async def test_say_hello(greeter_protos, grpc_service):
    request = greeter_protos.GreetingRequest(name='Python')
    response = await grpc_service.SayHello(request)
    assert response.greeting == 'Hello, Python!'


async def test_grpc(service_client):
    response = await service_client.post(
        '/hello', data='tests', headers={'Content-type': 'text/plain'},
    )
    assert response.status == 200
    assert response.content == b'Hello, tests!'
