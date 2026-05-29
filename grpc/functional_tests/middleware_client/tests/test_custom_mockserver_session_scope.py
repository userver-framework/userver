import grpc
import pytest
import pytest_userver.grpc

import samples.greeter_pb2 as greeter_protos
import samples.greeter_pb2_grpc as greeter_services


# /// [global mock]
@pytest.fixture(scope='module')
def global_grpc_mockserver(grpc_mockserver_session):
    with pytest_userver.grpc.Mockserver(
        mockserver_session=grpc_mockserver_session,
        experimental=True,
    ) as mockserver:
        yield mockserver


@pytest.fixture(scope='module', autouse=True)
def global_greeter_mock(global_grpc_mockserver):
    @global_grpc_mockserver(greeter_services.GreeterServiceServicer.SayHello)
    async def session_mock(request, context: grpc.aio.ServicerContext):
        return greeter_protos.GreetingResponse(greeting='Hello from session mock!')

    return session_mock
    # /// [global mock]


async def test_custom_session_mockserver_is_used_first(
    service_client,
    grpc_mockserver,
    global_greeter_mock,
):
    # Skip mock executions from previous tests.
    global_greeter_mock.flush()

    await service_client.run_task('call-say-hello')
    assert global_greeter_mock.times_called == 1

    @grpc_mockserver(greeter_services.GreeterServiceServicer.SayHello)
    async def test_mock(request, context: grpc.aio.ServicerContext):
        return greeter_protos.GreetingResponse(greeting='Hello from test mock!')

    await service_client.run_task('call-say-hello')

    assert test_mock.times_called == 1


async def test_custom_session_mockserver_is_restored(service_client, global_greeter_mock):
    # Skip mock executions from previous tests.
    global_greeter_mock.flush()

    await service_client.run_task('call-say-hello')

    assert global_greeter_mock.times_called == 1
