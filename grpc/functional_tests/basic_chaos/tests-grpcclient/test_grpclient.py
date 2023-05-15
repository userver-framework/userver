async def test_service_without_case(grpc_ch, service_client, gate):
    response = await service_client.post(
        '/hello', data='Python', headers={'Content-type': 'text/plain'},
    )
    assert response.status == 200
    assert response.content == b'Case not found'
    assert gate.connections_count() == 0


async def test_say_hello(grpc_ch, service_client, gate):
    response = await service_client.post(
        '/hello?case=say_hello',
        data='Python',
        headers={'Content-type': 'text/plain'},
    )
    assert response.status == 200
    assert response.content == b'Hello, Python!'
    assert gate.connections_count() > 0


async def test_say_hello_response_stream(grpc_ch, service_client, gate):
    response = await service_client.post(
        '/hello?case=say_hello_response_stream',
        data='Python',
        headers={'Content-type': 'text/plain'},
    )
    assert response.status == 200
    assert (
        response.content
        == b"""Hello, Python!
Hello, Python!!
Hello, Python!!!
Hello, Python!!!!
Hello, Python!!!!!
"""
    )
    assert gate.connections_count() > 0


async def test_say_hello_request_stream(grpc_ch, service_client, gate):
    response = await service_client.post(
        '/hello?case=say_hello_request_stream',
        data='Python\n!\n!\n!',
        headers={'Content-type': 'text/plain'},
    )
    assert response.status == 200
    assert response.content == b'Hello, Python!!!'
    assert gate.connections_count() > 0


async def test_say_hello_streams(grpc_ch, service_client, gate):
    response = await service_client.post(
        '/hello?case=say_hello_streams',
        data='Python\n!\n!\n!',
        headers={'Content-type': 'text/plain'},
    )
    assert response.status == 200
    assert (
        response.content
        == b"""Hello, Python
Hello, Python!
Hello, Python!!
Hello, Python!!!
"""
    )
    assert gate.connections_count() > 0


async def test_say_hello_indept_streams(grpc_ch, service_client, gate):
    data = 'If\n \nthis\n \nmessage\n \nhas\n \n'
    data += 'arrived\n \nthen\n \neverything\n \nworks'
    response = await service_client.post(
        '/hello?case=say_hello_indept_streams',
        data=data,
        headers={'Content-type': 'text/plain'},
    )
    assert response.status == 200
    assert (
        response.content
        == b"""Hello, Python
Hello, C++
Hello, linux
Hello, userver
Hello, grpc
Hello, kernel
Hello, developer
Hello, core
Hello, anonim
Hello, user
If this message has arrived then everything works
"""
    )
    assert gate.connections_count() > 0
