import pytest

from samples import greeter_pb2

UNSAMPLED_TRACE_ID = 'a0e1afed08e019fc1110464cfa66635c'
UNSAMPLED_TRACEPARENT = f'00-{UNSAMPLED_TRACE_ID}-7a085853722dc6d2-00'

SAMPLED_TRACE_ID = 'b0e1afed08e019fc1110464cfa66635c'
SAMPLED_TRACEPARENT = f'00-{SAMPLED_TRACE_ID}-7a085853722dc6d2-01'

TRACESTATE_TRACE_ID = 'c0e1afed08e019fc1110464cfa66635c'
TRACESTATE_TRACEPARENT = f'00-{TRACESTATE_TRACE_ID}-7a085853722dc6d2-01'
TRACESTATE_VALUE = 'vendor=abc,foo=bar'


async def test_grpc_unsampled_propagates_flag_to_outgoing_http(grpc_client, mockserver):
    outgoing_traceparent = None

    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        nonlocal outgoing_traceparent
        outgoing_traceparent = request.headers.get('traceparent', '')
        return mockserver.make_response()

    request = greeter_pb2.GreetingRequest(name='test')
    response = await grpc_client.CallEchoNobody(request, metadata=[('traceparent', UNSAMPLED_TRACEPARENT)])
    assert response.greeting == 'Call Echo Nobody'
    assert _handler.times_called == 1
    assert outgoing_traceparent, 'traceparent must be forwarded to HTTP child request'
    assert outgoing_traceparent.split('-')[3] == '00', (
        f'Child request must carry sampled=0, got: {outgoing_traceparent}'
    )


async def test_grpc_sampled_propagates_flag_to_outgoing_http(grpc_client, mockserver):
    outgoing_traceparent = None

    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        nonlocal outgoing_traceparent
        outgoing_traceparent = request.headers.get('traceparent', '')
        return mockserver.make_response()

    request = greeter_pb2.GreetingRequest(name='test')
    response = await grpc_client.CallEchoNobody(request, metadata=[('traceparent', SAMPLED_TRACEPARENT)])
    assert response.greeting == 'Call Echo Nobody'
    assert _handler.times_called == 1
    assert outgoing_traceparent, 'traceparent must be forwarded to HTTP child request'
    assert outgoing_traceparent.split('-')[3] == '01', (
        f'Child request must carry sampled=1, got: {outgoing_traceparent}'
    )


async def test_grpc_tracestate_propagated_to_outgoing_http(grpc_client, mockserver):
    outgoing_tracestate = None

    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        nonlocal outgoing_tracestate
        outgoing_tracestate = request.headers.get('tracestate', '')
        return mockserver.make_response()

    request = greeter_pb2.GreetingRequest(name='test')
    response = await grpc_client.CallEchoNobody(
        request,
        metadata=[('traceparent', TRACESTATE_TRACEPARENT), ('tracestate', TRACESTATE_VALUE)],
    )
    assert response.greeting == 'Call Echo Nobody'
    assert _handler.times_called == 1
    assert outgoing_tracestate == TRACESTATE_VALUE, (
        f'tracestate must be propagated to HTTP child request, got: {outgoing_tracestate!r}'
    )


async def test_grpc_unsampled_suppresses_trace(grpc_client, service_client, mockserver):
    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        return mockserver.make_response()

    async with service_client.capture_logs() as capture:
        request = greeter_pb2.GreetingRequest(name='test')
        response = await grpc_client.CallEchoNobody(request, metadata=[('traceparent', UNSAMPLED_TRACEPARENT)])
        assert response.greeting == 'Call Echo Nobody'

    assert _handler.times_called == 1
    trace_logs = capture.select(trace_id=UNSAMPLED_TRACE_ID)
    written_spans = [e.get('stopwatch_name') for e in trace_logs if 'stopwatch_name' in e]
    assert not written_spans, f'No trace spans must be written when sampled flag is 0, got: {written_spans}'


async def test_grpc_sampled_writes_trace(grpc_client, service_client, mockserver):
    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        return mockserver.make_response()

    async with service_client.capture_logs() as capture:
        request = greeter_pb2.GreetingRequest(name='test')
        response = await grpc_client.CallEchoNobody(request, metadata=[('traceparent', SAMPLED_TRACEPARENT)])
        assert response.greeting == 'Call Echo Nobody'

    assert _handler.times_called == 1
    trace_logs = capture.select(trace_id=SAMPLED_TRACE_ID)
    assert any('stopwatch_name' in e for e in trace_logs), 'Trace spans must be written when sampled flag is 1'


async def test_grpc_no_traceparent_creates_sampled_root(grpc_client, mockserver):
    outgoing_traceparent = None

    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        nonlocal outgoing_traceparent
        outgoing_traceparent = request.headers.get('traceparent', '')
        return mockserver.make_response()

    request = greeter_pb2.GreetingRequest(name='test')
    response = await grpc_client.CallEchoNobody(request, metadata=[])
    assert response.greeting == 'Call Echo Nobody'
    assert _handler.times_called == 1
    assert outgoing_traceparent, 'traceparent must be set for outgoing requests even without incoming traceparent'
    assert outgoing_traceparent.split('-')[3] == '01', (
        f'gRPC without traceparent must create sampled root span, got: {outgoing_traceparent}'
    )


@pytest.mark.uservice_oneshot(config_hooks=['disable_otel_trace_sampling'])
async def test_grpc_sampling_disabled_ignores_trace_flags(grpc_client, service_client, mockserver):
    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        return mockserver.make_response()

    async with service_client.capture_logs() as capture:
        request = greeter_pb2.GreetingRequest(name='test')
        response = await grpc_client.CallEchoNobody(
            request,
            metadata=[('traceparent', UNSAMPLED_TRACEPARENT)],
        )
        assert response.greeting == 'Call Echo Nobody'

    assert _handler.times_called >= 1
    trace_logs = capture.select(trace_id=UNSAMPLED_TRACE_ID)
    written_spans = [e.get('stopwatch_name') for e in trace_logs if 'stopwatch_name' in e]
    assert written_spans, (
        'Trace spans must be written when otel-trace-sampling-enabled is false, '
        f'even if trace-flags is 00, got: {written_spans}'
    )
