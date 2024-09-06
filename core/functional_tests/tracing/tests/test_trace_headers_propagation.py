B3_HEADERS = {
    'X-B3-TraceId': '10e1afed08e019fc1110464cfa66635c',
    'X-B3-SpanId': '7a085853722dc6d2',
    'X-B3-Sampled': '1',
}

OPENTELEMETRY_TRACE_ID = '20e1afed08e019fc1110464cfa66635c'
OPENTELEMETRY_HEADERS = {
    'traceparent': f'00-{OPENTELEMETRY_TRACE_ID}-7a085853722dc6d2-01',
    'tracestate': '42',
}

TAXI_HEADERS = {
    'X-YaTraceId': '30e1afed08e019fc1110464cfa66635c',
    'X-YaSpanId': '8a085853722dc6d2',
}

TAXI_HEADERS_EXT = {'X-YaRequestId': '40e1afed08e019fc1110464cfa66635c'}

YANDEX_HEADERS = {'X-RequestId': '50e1afed08e019fc1110464cfa66635c'}


async def test_empty_b3_tracing_headers(service_client, mockserver):
    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        assert 'X-B3-TraceId' in request.headers
        assert 'X-B3-Sampled' in request.headers
        return mockserver.make_response()

    response = await service_client.get('/echo-no-body')
    assert _handler.times_called >= 1
    assert response.status_code == 200


async def test_empty_otel_tracing_headers(service_client, mockserver):
    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        assert 'traceparent' in request.headers
        return mockserver.make_response()

    response = await service_client.get('/echo-no-body')
    assert _handler.times_called >= 1
    assert response.status_code == 200
    assert response.headers['traceparent'].split('-')[0] == '00'
    assert response.headers['traceparent'].split('-')[3] == '01'


async def test_empty_taxi_tracing_headers(service_client, mockserver):
    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        assert 'X-YaTraceId' in request.headers
        assert 'X-YaRequestId' in request.headers
        assert 'X-YaSpanId' in request.headers
        return mockserver.make_response()

    response = await service_client.get('/echo-no-body')
    assert _handler.times_called >= 1
    assert response.status_code == 200


async def test_empty_yandex_tracing_headers(service_client, mockserver):
    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        assert 'X-RequestId' in request.headers
        assert request.headers['X-RequestId'] == request.headers['X-YaTraceId']
        return mockserver.make_response()

    response = await service_client.get('/echo-no-body')
    assert _handler.times_called >= 1
    assert response.status_code == 200


async def test_b3_tracing_headers(
        service_client, mockserver, assert_ids_in_file,
):
    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        assert request.headers['X-YaTraceId'] == B3_HEADERS['X-B3-TraceId']
        assert request.headers['X-B3-TraceId'] == B3_HEADERS['X-B3-TraceId']
        assert request.headers['X-B3-Sampled'] == B3_HEADERS['X-B3-Sampled']
        assert request.headers['X-YaSpanId'] == request.headers['X-B3-SpanId']
        return mockserver.make_response()

    response = await service_client.get('/echo-no-body', headers=B3_HEADERS)
    assert _handler.times_called >= 1
    assert response.status_code == 200
    await assert_ids_in_file(B3_HEADERS['X-B3-TraceId'])


async def test_otel_tracing_headers(
        service_client, mockserver, assert_ids_in_file,
):
    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        assert request.headers['X-YaTraceId'] == OPENTELEMETRY_TRACE_ID
        assert (
            request.headers['traceparent'].split('-')[0]
            == OPENTELEMETRY_HEADERS['traceparent'].split('-')[0]
        )
        assert (
            request.headers['traceparent'].split('-')[1]
            == OPENTELEMETRY_HEADERS['traceparent'].split('-')[1]
        )
        assert (
            request.headers['traceparent'].split('-')[3]
            == OPENTELEMETRY_HEADERS['traceparent'].split('-')[3]
        ), request.headers['traceparent']
        assert (
            request.headers['tracestate']
            == OPENTELEMETRY_HEADERS['tracestate']
        )
        return mockserver.make_response()

    response = await service_client.get(
        '/echo-no-body', headers=OPENTELEMETRY_HEADERS,
    )
    assert _handler.times_called >= 1
    assert response.status_code == 200
    await assert_ids_in_file(OPENTELEMETRY_TRACE_ID)


async def test_taxi_tracing_headers(
        service_client, mockserver, assert_ids_in_file,
):
    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        assert request.headers['X-YaTraceId'] == TAXI_HEADERS['X-YaTraceId']
        assert 'X-YaRequestId' in request.headers
        assert request.headers['X-YaSpanId'] != TAXI_HEADERS['X-YaSpanId']
        return mockserver.make_response()

    response = await service_client.get('/echo-no-body', headers=TAXI_HEADERS)
    assert _handler.times_called >= 1
    assert response.status_code == 200
    await assert_ids_in_file(TAXI_HEADERS['X-YaTraceId'])


async def test_taxi_tracing_headers_ext(service_client, mockserver):
    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        assert request.headers['X-YaTraceId'] == TAXI_HEADERS['X-YaTraceId']
        assert (
            request.headers['X-YaRequestId']
            != TAXI_HEADERS_EXT['X-YaRequestId']
        )
        assert request.headers['X-YaSpanId'] != TAXI_HEADERS['X-YaSpanId']
        return mockserver.make_response()

    response = await service_client.get(
        '/echo-no-body', headers={**TAXI_HEADERS, **TAXI_HEADERS_EXT},
    )
    assert _handler.times_called >= 1
    assert response.status_code == 200


async def test_taxi_tracing_headers_min(service_client, mockserver):
    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        assert request.headers['X-YaTraceId'] == TAXI_HEADERS['X-YaTraceId']
        assert 'X-YaRequestId' in request.headers
        assert 'X-YaSpanId' in request.headers
        return mockserver.make_response()

    response = await service_client.get(
        '/echo-no-body', headers={'X-YaTraceId': TAXI_HEADERS['X-YaTraceId']},
    )
    assert _handler.times_called >= 1
    assert response.status_code == 200


async def test_yandex_tracing_headers(
        service_client, mockserver, assert_ids_in_file,
):
    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        assert request.headers['X-RequestId'] == YANDEX_HEADERS['X-RequestId']
        assert request.headers['X-RequestId'] == request.headers['X-YaTraceId']
        return mockserver.make_response()

    response = await service_client.get(
        '/echo-no-body', headers=YANDEX_HEADERS,
    )
    assert _handler.times_called >= 1
    assert response.status_code == 200
    await assert_ids_in_file(YANDEX_HEADERS['X-RequestId'])


async def test_priority_otel_tracing_headers(service_client, mockserver):
    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        assert request.headers['X-YaTraceId'] == OPENTELEMETRY_TRACE_ID
        return mockserver.make_response()

    response = await service_client.get(
        '/echo-no-body', headers={**TAXI_HEADERS, **OPENTELEMETRY_HEADERS},
    )
    assert _handler.times_called >= 1
    assert response.status_code == 200


async def test_priority_b3_tracing_headers(service_client, mockserver):
    @mockserver.json_handler('/test-service/echo-no-body')
    async def _handler(request):
        assert request.headers['X-YaTraceId'] == B3_HEADERS['X-B3-TraceId']
        return mockserver.make_response()

    response = await service_client.get(
        '/echo-no-body', headers={**TAXI_HEADERS, **B3_HEADERS},
    )
    assert _handler.times_called >= 1
    assert response.status_code == 200
