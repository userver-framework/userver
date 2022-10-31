async def test_say_hello(greeter_protos, grpc_service):
    request = greeter_protos.GreetingRequest(name='Python')
    response = await grpc_service.SayHello(request)
    assert response.greeting == 'Hello, Python!'


async def test_grpc(service_client, greeter_protos, mock_grpc_greeter):
    @mock_grpc_greeter('SayHello')
    async def mock_say_hello(request, context):
        return greeter_protos.GreetingResponse(
            greeting=f'Hello, {request.name} from mockserver!',
        )

    response = await service_client.post(
        '/hello', data='tests', headers={'Content-type': 'text/plain'},
    )
    assert response.status == 200
    assert response.content == b'Hello, tests from mockserver!'

    assert mock_say_hello.times_called == 1
