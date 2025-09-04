import grpc
import pytest
import pytest_userver.client

import samples.greeter_pb2 as greeter_protos
import samples.greeter_pb2_grpc as greeter_services


# /// [Mocked error status]
async def test_error_status(service_client, grpc_mockserver):
    @grpc_mockserver(greeter_services.GreeterServiceServicer.SayHello)
    async def mock_say_hello(request, context: grpc.aio.ServicerContext):
        # Don't forget the `await`!
        await context.abort(grpc.StatusCode.UNAVAILABLE, 'Greeter is down')

    with pytest.raises(pytest_userver.client.TestsuiteTaskFailed) as ex_info:
        await service_client.run_task('call-say-hello')
    ex_info.match("'samples.api.GreeterService/SayHello' failed: code=UNAVAILABLE, message='Greeter is down'")
    assert mock_say_hello.times_called == 2
    # /// [Mocked error status]


async def test_error_status_via_result(service_client, grpc_mockserver):
    @grpc_mockserver(greeter_services.GreeterServiceServicer.SayHello)
    async def mock_say_hello(request, context: grpc.aio.ServicerContext):
        # Be aware of using `context.set_code()`!
        # Maybe you should use `await context.abort()` instead
        # e.g. using `context.set_code()` as an unexpected side effect disables grpc-core client retries
        # but does not disable userver grpc retries
        context.set_code(grpc.StatusCode.UNAVAILABLE)
        context.set_details('Greeter is down')
        return greeter_protos.GreetingResponse()  # Message is ignored.

    with pytest.raises(pytest_userver.client.TestsuiteTaskFailed) as ex_info:
        await service_client.run_task('call-say-hello')
    ex_info.match("'samples.api.GreeterService/SayHello' failed: code=UNAVAILABLE, message='Greeter is down'")
    # using `context.set_code()` does not disable userver grpc retries
    assert mock_say_hello.times_called == 2
