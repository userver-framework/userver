import grpc

import samples.greeter_pb2 as greeter_protos
import samples.greeter_pb2_grpc as greeter_services


async def test_headers_propagator(service_client, grpc_mockserver):
    @grpc_mockserver(greeter_services.GreeterServiceServicer.SayHello)
    async def mock_say_hello(request, context: grpc.aio.ServicerContext):
        req_metadata = grpc.aio.Metadata(*context.invocation_metadata())

        assert req_metadata.get('skipped-header') is None
        assert req_metadata['allowed-header'] == 'v2'
        assert req_metadata['x-upper-case-header'] == 'v3'

        return greeter_protos.GreetingResponse()

    await service_client.run_task('call-say-hello-headers-propagator')
