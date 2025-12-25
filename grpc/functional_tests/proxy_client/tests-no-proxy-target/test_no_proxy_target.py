import grpc
import pytest

import samples.greeter_pb2 as greeter_protos
import samples.greeter_pb2_grpc as greeter_services


@pytest.mark.parametrize(
    'cfg_enabled',
    [False, True],
)
@pytest.mark.parametrize(
    'no_proxy_target',
    [False, True],
)
async def test_no_proxy_target_by_file(
    service_client,
    grpc_mockserver,
    taxi_config,
    grpc_mockserver_endpoint,
    cfg_enabled,
    no_proxy_target,
):
    taxi_config.set_values({
        'EGRESS_GRPC_PROXY_ENABLED': cfg_enabled,
        'EGRESS_NO_PROXY_TARGETS': {
            'targets': [grpc_mockserver_endpoint] if no_proxy_target else [],
        },
    })
    await service_client.update_server_state()

    @grpc_mockserver(greeter_services.GreeterServiceServicer.SayHello)
    async def mock_say_hello(request, context: grpc.aio.ServicerContext):
        return greeter_protos.GreetingResponse()

    await service_client.run_task('call-say-hello')
    assert mock_say_hello.times_called == 1
