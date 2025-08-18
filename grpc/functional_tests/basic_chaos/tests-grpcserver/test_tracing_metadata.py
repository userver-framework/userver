import pytest

import samples.greeter_pb2 as greeter_pb2  # noqa: E402, E501


@pytest.mark.parametrize(
    'metadata,logs,',
    [
        [(('x-yatraceid', 'traceid'),), {'trace_id': 'traceid'}],
        [
            (('x-yatraceid', 'traceid'), ('x-yaspanid', 'span')),
            {'trace_id': 'traceid', 'parent_id': 'span'},
        ],
    ],
)
async def test_tracing_metadata(
    grpc_client,
    service_client,
    gate,
    metadata,
    logs,
):
    request = greeter_pb2.GreetingRequest(name='Python')
    async with service_client.capture_logs() as capture:
        response = await grpc_client.SayHello(
            request,
            wait_for_ready=True,
            metadata=metadata,
        )
        assert response.greeting == 'Hello, Python!'

    assert capture.select(**logs)
