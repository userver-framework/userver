import pytest


@pytest.mark.parametrize(
    'status_code,expected_code,is_error',
    [(200, None, False), (404, None, True), (404, 404, False), (500, 404, True)],
)
async def test_expected_status_codes(service_client, mockserver, status_code, expected_code, is_error):
    @mockserver.handler('/test-service/echo-no-body')
    async def upstream(request):
        return mockserver.make_response('', status=status_code)

    params = {} if expected_code is None else {'expected_code': str(expected_code)}
    async with service_client.capture_logs() as capture:
        response = await service_client.get('/echo-no-body', params=params)
        assert response.status_code == (200 if status_code < 400 else 500)

    assert upstream.times_called == (2 if status_code >= 500 else 1)
    spans = capture.select(stopwatch_name='GET /expected-status-code-test')
    assert len(spans) == 1
    span = spans[0]
    assert span['meta_code'] == str(status_code)
    if is_error:
        assert span['error'] == '1'
    else:
        assert 'error' not in span
