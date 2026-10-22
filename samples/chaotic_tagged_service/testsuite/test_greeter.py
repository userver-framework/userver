async def test_greeting_is_passed_through(service_client, mockserver):
    @mockserver.json_handler('/upstream/greetings/john')
    def upstream(request):
        return {'name': 'john', 'text': 'Hello, john!'}

    response = await service_client.get('/greetings/john')
    assert response.status == 200
    assert response.json() == {'name': 'john', 'text': 'Hello, john!'}
    assert upstream.times_called == 1


async def test_error_is_passed_through(service_client, mockserver):
    @mockserver.json_handler('/upstream/greetings/nobody')
    def upstream(request):
        return mockserver.make_response(json={'code': 'not_found', 'message': 'Nobody here'}, status=404)

    response = await service_client.get('/greetings/nobody')
    assert response.status == 404
    assert response.json() == {'code': 'not_found', 'message': 'Nobody here'}
    assert upstream.times_called == 1
