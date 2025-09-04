# grpc template on
# Start the tests via `make test-debug` or `make test-release`

import handlers.hello_pb2 as hello_protos  # noqa: E402, E501


async def test_grpc_service(grpc_service):
    request = hello_protos.HelloRequest(name='Tester')
    response = await grpc_service.SayHello(request)
    assert response.text == 'Hello, Tester!\n'


# grpc template off
