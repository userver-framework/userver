import pytest

UNSAMPLED_TRACE_ID = '40e1afed08e019fc1110464cfa66635c'
UNSAMPLED_HEADERS = {
    'traceparent': f'00-{UNSAMPLED_TRACE_ID}-7a085853722dc6d2-00',
}

SAMPLED_TRACE_ID = '50e1afed08e019fc1110464cfa66635c'
SAMPLED_HEADERS = {
    'traceparent': f'00-{SAMPLED_TRACE_ID}-7a085853722dc6d2-01',
}

TRACESTATE_TRACE_ID = '60e1afed08e019fc1110464cfa66635c'
TRACESTATE_UNSAMPLED_HEADERS = {
    'traceparent': f'00-{TRACESTATE_TRACE_ID}-7a085853722dc6d2-00',
    'tracestate': 'vendor=abc,foo=bar',
}

RESPONSE_TRACE_ID = '70e1afed08e019fc1110464cfa66635c'
RESPONSE_SAMPLED_HEADERS = {
    'traceparent': f'00-{RESPONSE_TRACE_ID}-7a085853722dc6d2-01',
}


async def test_otel_unsampled_logs_not_suppressed(service_client, mockserver):
    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        return mockserver.make_response()

    async with service_client.capture_logs() as capture:
        response = await service_client.get('/echo-no-body', headers=UNSAMPLED_HEADERS)
        assert response.status_code == 200

    assert _handler.times_called >= 1
    trace_logs = capture.select(trace_id=UNSAMPLED_TRACE_ID)

    written_spans = [e.get('stopwatch_name') for e in trace_logs if 'stopwatch_name' in e]
    assert not written_spans, f'Span must not be written when sampled=0, got: {written_spans}'

    handler_logs = [e for e in trace_logs if e.get('text') == 'echo-no-body handler called']
    assert handler_logs, 'LOG_INFO() inside an unsampled handler must not be suppressed'


async def test_otel_unsampled_suppresses_trace(service_client, mockserver):
    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        return mockserver.make_response()

    async with service_client.capture_logs() as capture:
        response = await service_client.get('/echo-no-body', headers=UNSAMPLED_HEADERS)
        assert response.status_code == 200

    assert _handler.times_called >= 1
    trace_logs = capture.select(trace_id=UNSAMPLED_TRACE_ID)
    written_spans = [e.get('stopwatch_name') for e in trace_logs if 'stopwatch_name' in e]
    assert not written_spans, f'No trace spans must be written when sampled flag is 0, got: {written_spans}'


async def test_otel_unsampled_propagates_flag_to_child(service_client, mockserver):
    outgoing_traceparent = None

    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        nonlocal outgoing_traceparent
        outgoing_traceparent = request.headers.get('traceparent', '')
        return mockserver.make_response()

    response = await service_client.get('/echo-no-body', headers=UNSAMPLED_HEADERS)
    assert response.status_code == 200
    assert _handler.times_called >= 1
    assert outgoing_traceparent, 'traceparent header must be forwarded to child requests'
    assert outgoing_traceparent.split('-')[3] == '00', (
        f'Child traceparent must carry sampled=0, got: {outgoing_traceparent}'
    )


async def test_otel_sampled_writes_trace(service_client, mockserver):
    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        return mockserver.make_response()

    async with service_client.capture_logs() as capture:
        response = await service_client.get('/echo-no-body', headers=SAMPLED_HEADERS)
        assert response.status_code == 200

    assert _handler.times_called >= 1
    trace_logs = capture.select(trace_id=SAMPLED_TRACE_ID)
    assert any('stopwatch_name' in entry for entry in trace_logs), 'Trace spans must be written when sampled flag is 1'


async def test_otel_tracestate_propagated_when_unsampled(service_client, mockserver):
    outgoing_tracestate = None

    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        nonlocal outgoing_tracestate
        outgoing_tracestate = request.headers.get('tracestate', '')
        return mockserver.make_response()

    response = await service_client.get('/echo-no-body', headers=TRACESTATE_UNSAMPLED_HEADERS)
    assert response.status_code == 200
    assert _handler.times_called >= 1
    assert outgoing_tracestate == TRACESTATE_UNSAMPLED_HEADERS['tracestate'], (
        f'tracestate must be propagated unchanged even when unsampled, got: {outgoing_tracestate!r}'
    )


async def test_otel_no_traceparent_creates_sampled_root(service_client, mockserver):
    outgoing_traceparent = None

    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        nonlocal outgoing_traceparent
        outgoing_traceparent = request.headers.get('traceparent', '')
        return mockserver.make_response()

    response = await service_client.get('/echo-no-body')
    assert response.status_code == 200
    assert _handler.times_called >= 1
    assert outgoing_traceparent, 'traceparent must be present in outgoing request even without incoming OTel headers'
    assert outgoing_traceparent.split('-')[3] == '01', (
        f'Root span without OTel headers must default to sampled=1, got: {outgoing_traceparent}'
    )


async def test_otel_response_contains_traceparent(service_client, mockserver):
    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        return mockserver.make_response()

    response = await service_client.get('/echo-no-body', headers=RESPONSE_SAMPLED_HEADERS)
    assert response.status_code == 200
    assert _handler.times_called >= 1
    response_traceparent = response.headers.get('traceparent', '')
    assert response_traceparent, 'Response must contain traceparent header'
    assert response_traceparent.split('-')[1] == RESPONSE_TRACE_ID, (
        f'Response traceparent must carry the incoming trace_id, got: {response_traceparent}'
    )


@pytest.mark.uservice_oneshot(config_hooks=['disable_otel_trace_sampling'])
async def test_otel_trace_sampling_disabled_ignores_trace_flags(service_client, mockserver):
    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        return mockserver.make_response()

    async with service_client.capture_logs() as capture:
        response = await service_client.get('/echo-no-body', headers=UNSAMPLED_HEADERS)
        assert response.status_code == 200

    assert _handler.times_called >= 1
    trace_logs = capture.select(trace_id=UNSAMPLED_TRACE_ID)
    written_spans = [e.get('stopwatch_name') for e in trace_logs if 'stopwatch_name' in e]
    assert written_spans, (
        'Trace spans must be written when otel-trace-sampling-enabled is false, '
        f'even if trace-flags is 00, got: {written_spans}'
    )


@pytest.mark.parametrize('task_mode', ['async', 'background', 'detached'])
@pytest.mark.parametrize(
    'sampling_enabled',
    [
        pytest.param(True, id='sampling-enabled'),
        pytest.param(
            False,
            id='sampling-disabled',
            marks=pytest.mark.uservice_oneshot(config_hooks=['disable_otel_trace_sampling']),
        ),
    ],
)
@pytest.mark.parametrize('trace_flags', ['00', '02', '01', '03', None, 'invalid'])
async def test_otel_task_propagation(
    service_client,
    mockserver,
    task_mode,
    sampling_enabled,
    trace_flags,
):
    trace_id = '80e1afed08e019fc1110464cfa66635c'
    headers = {}
    expected_flags = '01'
    expected_tracestate = ''
    if trace_flags is not None:
        headers['traceparent'] = f'00-{trace_id}-7a085853722dc6d2-{trace_flags}'
        headers['tracestate'] = 'vendor=abc,foo=bar'
        if trace_flags != 'invalid':
            expected_flags = trace_flags
            expected_tracestate = headers['tracestate']

    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        outgoing = request.headers['traceparent'].split('-')
        assert outgoing[3] == expected_flags
        if expected_tracestate:
            assert outgoing[1] == trace_id
        assert request.headers.get('tracestate', '') == expected_tracestate
        return mockserver.make_response()

    async with service_client.capture_logs() as capture:
        response = await service_client.get(
            '/echo-no-body',
            params={'task_mode': task_mode},
            headers=headers,
        )
        assert response.status_code == 200

    assert _handler.times_called == 1
    handler_logs = capture.select(text='echo-no-body handler called')
    assert len(handler_logs) == 1
    actual_trace_id = handler_logs[0]['trace_id']
    if expected_tracestate:
        assert actual_trace_id == trace_id
    trace_logs = capture.select(trace_id=actual_trace_id)
    written_spans = [entry for entry in trace_logs if 'stopwatch_name' in entry]
    expected_sampled = not sampling_enabled or bool(int(expected_flags, 16) & 1)
    assert bool(written_spans) == expected_sampled
