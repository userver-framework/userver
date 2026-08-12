import pytest

import samples.greeter_pb2 as greeter_pb2


def issubset(needle: dict[str, object], haystack: dict[str, object]) -> bool:
    return haystack | needle == haystack


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
    async with service_client.capture_logs() as capture:

        @capture.subscribe(trace_id='traceid', stopwatch_units='ms')
        def server_span(**tags):
            pass

        response = await grpc_client.SayHello(greeter_pb2.GreetingRequest(name='Python'), metadata=metadata)
        assert response.greeting == 'Hello, Python!'

        # Server span is written after response is sent to the client, so we have to wait until the span appears.
        call = await server_span.wait_call()
        assert issubset(logs, call['tags'])
