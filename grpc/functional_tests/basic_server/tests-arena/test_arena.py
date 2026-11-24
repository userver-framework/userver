import samples.greeter_pb2 as greeter_protos


async def test_arena_requests_and_responses(grpc_client, testpoint, arena_config):
    @testpoint('grpc-arenas')
    def on_arenas(data):
        enabled = arena_config[2]
        assert data == {'request': enabled, 'response': enabled}

    for name in ('Python', 'x' * 4096):
        response = await grpc_client.SayHello(greeter_protos.GreetingRequest(name=name))
        assert response.greeting == f'Hello, {name}!'

    assert on_arenas.times_called == 2
