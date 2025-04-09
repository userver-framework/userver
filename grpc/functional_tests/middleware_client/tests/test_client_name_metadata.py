import grpc

import samples.greeter_pb2 as greeter_protos
import samples.greeter_pb2_grpc as greeter_services

CLIENT_NAME_METADATA = 'x-testsuite-client-name'


async def test_error_status(service_client, grpc_mockserver):
    @grpc_mockserver(greeter_services.GreeterServiceServicer.SayHello)
    async def mock_say_hello(request, context: grpc.aio.ServicerContext):
        req_metadata = grpc.aio.Metadata(*context.invocation_metadata())
        # Client name is equal to the component name by default.
        # "-client" suffix is cut off, if present.
        assert req_metadata[CLIENT_NAME_METADATA] == 'greeter'
        return greeter_protos.GreetingResponse()

    await service_client.run_task('call-say-hello')
