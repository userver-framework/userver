from grpc import StatusCode
from grpc.aio import AioRpcError
import pytest

import samples.greeter_pb2 as greeter_protos


async def test_meta_filter(grpc_client):
    request = greeter_protos.GreetingRequest(name='Python')

    with pytest.raises(AioRpcError) as err:
        await grpc_client.SayHello(
            request=request,
            metadata=[
                ('x-key', 'secret-credentials'),
                ('bad-header', 'bad-value'),
            ],
        )
    assert err.value.code() == StatusCode.FAILED_PRECONDITION


# /// [grpc authentication tests]
async def test_correct_credentials(grpc_client):
    request = greeter_protos.GreetingRequest(name='Python')
    response = await grpc_client.SayHello(
        request=request,
        metadata=[
            ('x-key', 'secret-credentials'),
            ('specific-header', 'specific-value'),
        ],
    )
    assert response.greeting == 'Hello, Python!'


async def test_incorrect_credentials(grpc_client):
    request = greeter_protos.GreetingRequest(name='Python')

    with pytest.raises(AioRpcError) as err:
        await grpc_client.SayHello(
            request=request,
            metadata=[('x-key', 'secretcredentials')],
        )
    assert err.value.code() == StatusCode.PERMISSION_DENIED


async def test_no_credentials(grpc_client):
    request = greeter_protos.GreetingRequest(name='Python')

    with pytest.raises(AioRpcError) as err:
        await grpc_client.SayHello(request=request)
    assert err.value.code() == StatusCode.PERMISSION_DENIED


# /// [grpc authentication tests]
