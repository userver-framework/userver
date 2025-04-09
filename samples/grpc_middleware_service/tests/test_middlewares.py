from grpc import StatusCode
from grpc.aio import AioRpcError
import pytest

# /// [gRPC mockserver]
import samples.greeter_pb2 as greeter_protos
import samples.greeter_pb2_grpc as greeter_services


async def test_basic_mock_server(service_client, grpc_mockserver):
    @grpc_mockserver(greeter_services.GreeterServiceServicer.SayHello)
    async def mock_say_hello(request, context):
        return greeter_protos.GreetingResponse(
            greeting=f'Hello, {request.name} from mockserver!',
        )

    response = await service_client.post('/hello?case=say_hello', data='tests')
    assert response.status == 200
    assert response.text == 'Hello, tests from mockserver!'
    assert mock_say_hello.times_called == 1


# /// [gRPC mockserver]


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
