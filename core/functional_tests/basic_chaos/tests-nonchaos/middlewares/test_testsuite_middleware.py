import json


async def test_testsuite_middleware(service_client):
    testsuite_response = {
        'code': '500',
        'message': 'Internal Server Error',
        'details': 'some runtime error',
    }

    async with service_client.capture_logs() as capture:
        response = await service_client.get('/chaos/httpserver-with-exception')
        assert response.status_code == 500
        assert response.json() == testsuite_response

    logs = capture.select(level='ERROR')
    assert len(logs) == 2
    assert (
        logs[0]['text']
        == 'exception in \'handler-chaos-httpserver-with-exception\' handler: some runtime error (std::runtime_error)'
    )

    assert json.loads(logs[1]['body']) == testsuite_response
