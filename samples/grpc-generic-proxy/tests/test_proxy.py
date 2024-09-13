import grpc
import grpc.aio
import pytest

import samples.greeter_pb2 as greeter_protos  # noqa: E402, E501


async def test_basic(grpc_client, mock_grpc_greeter, service_client):
    @mock_grpc_greeter('SayHello')
    async def mock_say_hello(request, context):
        return greeter_protos.GreetingResponse(
            greeting=f'Hello, {request.name} from mockserver!',
        )

    request = greeter_protos.GreetingRequest(name='Python')
    response = await grpc_client.SayHello(request)
    assert mock_say_hello.times_called == 1
    assert response.greeting == 'Hello, Python from mockserver!'


async def test_request_metadata(
    grpc_client, mock_grpc_greeter, service_client,
):
    @mock_grpc_greeter('SayHello')
    async def mock_say_hello(request, context):
        req_metadata = grpc.aio.Metadata(*context.invocation_metadata())
        if (
            req_metadata.get('routing-param', '') != 'request-meta-value'
            or req_metadata.get('auth-key', '') != 'secret-stuff'
        ):
            raise ValueError('Failed to proxy the required metadata')
        if req_metadata.get('proxy-name', '') != 'grpc-generic-proxy':
            raise ValueError('Proxy failed to add custom metadata')

        return greeter_protos.GreetingResponse(greeting='Hi!')

    request = greeter_protos.GreetingRequest(name='Python')
    response = await grpc_client.SayHello(
        request,
        metadata=(
            ('routing-param', 'request-meta-value'),
            ('auth-key', 'secret-stuff'),
        ),
    )
    assert mock_say_hello.times_called == 1
    assert response.greeting == 'Hi!'


async def test_trailing_response_metadata(
    grpc_client, mock_grpc_greeter, service_client,
):
    @mock_grpc_greeter('SayHello')
    async def mock_say_hello(request, context):
        context.set_trailing_metadata((
            ('response-meta-key', 'response-meta-value'),
        ))
        return greeter_protos.GreetingResponse(greeting='Hi!')

    request = greeter_protos.GreetingRequest(name='Python')
    call = grpc_client.SayHello(request)
    response = await call
    assert mock_say_hello.times_called == 1
    assert response.greeting == 'Hi!'
    resp_metadata = await call.trailing_metadata()
    assert resp_metadata.get('response-meta-key', '') == 'response-meta-value'


async def test_error(grpc_client, mock_grpc_greeter, service_client):
    @mock_grpc_greeter('SayHello')
    async def mock_say_hello(request, context):
        context.set_code(grpc.StatusCode.UNAVAILABLE)
        context.set_details(f'Failed to greet {request.name}')
        context.set_trailing_metadata((
            ('response-meta-key', 'response-meta-value'),
        ))
        return greeter_protos.GreetingResponse()

    request = greeter_protos.GreetingRequest(name='Python')
    call = grpc_client.SayHello(request)
    with pytest.raises(grpc.aio.AioRpcError) as e:
        _ = await call
    assert e.value.code() == grpc.StatusCode.UNAVAILABLE
    assert e.value.details() == 'Failed to greet Python'
    resp_metadata = await call.trailing_metadata()
    assert resp_metadata.get('response-meta-key', '') == 'response-meta-value'
