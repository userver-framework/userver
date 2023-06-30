import sys

import pytest
import samples.greeter_pb2 as greeter_protos  # noqa: E402, E501


# /// [grpc client test]
async def test_grpc_client(service_client, mock_grpc_greeter):
    @mock_grpc_greeter('SayHello')
    async def _mock_say_hello(request, context):
        return greeter_protos.GreetingResponse(
            greeting=f'Hello, {request.name} from mockserver!',
        )

    response = await service_client.post(
        '/hello', data='tests', headers={'Content-type': 'text/plain'},
    )
    assert response.status == 200
    assert response.content == b'Hello, tests from mockserver!'

    assert _mock_say_hello.times_called == 1
    # /// [grpc client test]


@pytest.mark.skipif(
    sys.platform == 'darwin', reason='this test fails in old packages',
)
# /// [grpc server test]
async def test_say_hello(grpc_client):
    request = greeter_protos.GreetingRequest(name='Python')
    response = await grpc_client.SayHello(request)
    assert response.greeting == 'Hello, Python!'
    # /// [grpc server test]
