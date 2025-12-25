import grpc
import pytest
import pytest_userver.grpc

import samples.greeter_pb2 as greeter_protos
import samples.greeter_pb2_grpc as greeter_services


@pytest.fixture(scope='session')
async def grpc_mockserver_session(grpc_proxy_endpoint):
    server = grpc.aio.server()
    server.add_insecure_port(grpc_proxy_endpoint)

    async with pytest_userver.grpc.MockserverSession(server=server, experimental=True) as mockserver:
        yield mockserver


@pytest.fixture(autouse=True)
async def mock_say_hello(grpc_mockserver):
    @grpc_mockserver(greeter_services.GreeterServiceServicer.SayHello)
    async def mock_say_hello(request, context: grpc.aio.ServicerContext):
        # TODO check authority - somewhy grpc.default_authority from channel args is missing in context
        return greeter_protos.GreetingResponse()

    return mock_say_hello


@pytest.mark.config(EGRESS_GRPC_PROXY_ENABLED=True)
async def test_proxy_available_ok(service_client, mock_say_hello):
    await service_client.run_task('call-say-hello')
    assert mock_say_hello.times_called == 1


@pytest.mark.parametrize(
    ['disable_proxy', 'disable_target'],
    [
        (False, True),
        (True, False),
    ],
)
async def test_client_unavailable_proxy_disabled(
    service_client,
    mock_say_hello,
    taxi_config,
    grpc_mockserver_endpoint,
    disable_proxy,
    disable_target,
):
    taxi_config.set_values({
        'EGRESS_GRPC_PROXY_ENABLED': not disable_proxy,
        'EGRESS_NO_PROXY_TARGETS': {
            'targets': [grpc_mockserver_endpoint] if disable_target else [],
        },
    })
    await service_client.update_server_state()
    with pytest.raises(pytest_userver.client.TestsuiteTaskFailed, match='code=UNAVAILABLE'):
        await service_client.run_task('call-say-hello')
    assert mock_say_hello.times_called == 0
