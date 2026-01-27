from collections.abc import AsyncIterator

import grpc
import pytest
import pytest_userver.client
from typing_extensions import override

from samples import greeter_pb2
from samples import greeter_pb2_grpc


# /// [servicer]
class GreeterServicerImpl(greeter_pb2_grpc.GreeterServiceServicer):
    def __init__(self) -> None:
        self.name: str = 'testsuite'
        self.should_fail: bool = False

    @override
    async def SayHello(
        self, request: greeter_pb2.GreetingRequest, context: grpc.aio.ServicerContext
    ) -> greeter_pb2.GreetingResponse:
        if self.should_fail:
            await context.abort(code=grpc.StatusCode.UNAVAILABLE, details='Greeter is down')
        return greeter_pb2.GreetingResponse(greeting=f'Hello, {self.name}!')

    @override
    async def SayHelloRequestStream(
        self, request_iterator: AsyncIterator[greeter_pb2.GreetingRequest], context: grpc.aio.ServicerContext
    ) -> greeter_pb2.GreetingResponse:
        names = []
        async for request in request_iterator:
            names.append(request.name)
        return greeter_pb2.GreetingResponse(greeting=f'Hello, {" and ".join(names)}')
        # /// [servicer]


# /// [install servicer]
@pytest.fixture(name='greeter_mock')
def _greeter_mock(grpc_mockserver) -> GreeterServicerImpl:
    # install_servicer returns a proxy with callqueue functionality for each gRPC method.
    return grpc_mockserver.install_servicer(GreeterServicerImpl())
    # /// [install servicer]


# /// [use mock]
async def test_install_servicer(service_client, greeter_mock):
    # Get the service's client to visit grpc_mockserver.
    await service_client.run_task('call-say-hello')

    # Member access is proxied to the servicer impl.
    greeter_mock.should_fail = True

    with pytest.raises(pytest_userver.client.TestsuiteTaskFailed, match='Greeter is down'):
        await service_client.run_task('call-say-hello')

    # gRPC method implementations are replaced with callqueues.
    assert greeter_mock.SayHello.times_called == 3
    # /// [use mock]
