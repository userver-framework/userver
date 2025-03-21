import pytest
import pytest_userver.client
import pytest_userver.grpc

import samples.greeter_pb2_grpc as greeter_services


# /// [Mocked network failure]
async def test_network_fail(service_client, grpc_mockserver):
    @grpc_mockserver(greeter_services.GreeterServiceServicer.SayHello)
    async def mock_say_hello(request, context):
        raise pytest_userver.grpc.NetworkError()

    with pytest.raises(pytest_userver.client.TestsuiteTaskFailed) as ex_info:
        await service_client.run_task('call-say-hello')
    ex_info.match("'samples.api.GreeterService/SayHello' failed: interrupted at Testsuite network")
    # /// [Mocked network failure]


# /// [Mocked timeout failure]
async def test_timeout_fail(service_client, grpc_mockserver):
    @grpc_mockserver(greeter_services.GreeterServiceServicer.SayHello)
    async def mock_say_hello(request, context):
        raise pytest_userver.grpc.TimeoutError()

    with pytest.raises(pytest_userver.client.TestsuiteTaskFailed) as ex_info:
        await service_client.run_task('call-say-hello')
    ex_info.match("'samples.api.GreeterService/SayHello' failed: interrupted at Testsuite timeout")
    # /// [Mocked timeout failure]
