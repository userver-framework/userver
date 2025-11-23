import pytest

import samples.greeter_pb2 as greeter_pb2


@pytest.mark.parametrize(
    'metadata,logs',
    [
        [(('x-yatraceid', 'traceid'),), {'trace_id': 'traceid'}],
        [
            (('x-yatraceid', 'traceid'), ('x-yaspanid', 'span')),
            {'trace_id': 'traceid', 'parent_id': 'span'},
        ],
        [
            (('x-yatraceid', 'traceid'), ('x-yarequestid', 'request_id_value')),
            {'trace_id': 'traceid', 'parent_link': 'request_id_value'},
        ],
        [
            (('x-yatraceid', 'traceid'), ('x-yaspanid', 'span'), ('x-yarequestid', 'request_id_value')),
            {'trace_id': 'traceid', 'parent_id': 'span', 'parent_link': 'request_id_value'},
        ],
    ],
)
async def test_tracing_metadata(grpc_client, service_client, metadata, logs):
    request = greeter_pb2.GreetingRequest(name='Python')
    async with service_client.capture_logs() as capture:
        response = await grpc_client.SayHello(
            request,
            wait_for_ready=True,
            metadata=metadata,
        )
        assert response.greeting == 'Hello, Python!'

    assert capture.select(**logs)
