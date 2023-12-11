from grpc import StatusCode
from grpc.aio._call import AioRpcError
import samples.greeter_pb2 as greeter_protos  # noqa: E402, E501


# /// [grpc authentification tests]
async def test_correct_credentials(grpc_client):
    request = greeter_protos.GreetingRequest(name='Python')
    response = await grpc_client.SayHello(
        request=request, metadata=[('x-key', 'secret-credentials')],
    )
    assert response.greeting == 'Hello, Python!'


async def test_incorrect_credentials(grpc_client):
    request = greeter_protos.GreetingRequest(name='Python')

    try:
        await grpc_client.SayHello(
            request=request, metadata=[('x-key', 'secretcredentials')],
        )
        assert False
    except AioRpcError as err:
        assert err.code() == StatusCode.PERMISSION_DENIED


async def test_no_credentials(grpc_client):
    request = greeter_protos.GreetingRequest(name='Python')

    try:
        await grpc_client.SayHello(request=request)
        assert False
    except AioRpcError as err:
        assert err.code() == StatusCode.PERMISSION_DENIED


# /// [grpc authentification tests]
