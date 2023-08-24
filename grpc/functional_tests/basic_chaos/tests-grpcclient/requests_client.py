import asyncio

ALL_CASES = [
    'say_hello',
    'say_hello_response_stream',
    'say_hello_request_stream',
    'say_hello_streams',
    'say_hello_indept_streams',
]


async def _request_without_case(grpc_ch, service_client, gate):
    response = await service_client.post(
        '/hello', data='Python', headers={'Content-type': 'text/plain'},
    )
    assert response.status == 200
    assert response.content == b'Case not found'


async def _say_hello(grpc_ch, service_client, gate):
    response = await service_client.post(
        '/hello?case=say_hello',
        data='Python',
        headers={'Content-type': 'text/plain'},
    )
    assert response.status == 200
    assert response.content == b'Hello, Python!'
    assert gate.connections_count() > 0


async def _say_hello_response_stream(grpc_ch, service_client, gate):
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


async def _say_hello_request_stream(grpc_ch, service_client, gate):
    response = await service_client.post(
        '/hello?case=say_hello_request_stream',
        data='Python\n!\n!\n!',
        headers={'Content-type': 'text/plain'},
    )
    assert response.status == 200
    assert response.content == b'Hello, Python!!!'
    assert gate.connections_count() > 0


async def _say_hello_streams(grpc_ch, service_client, gate):
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


async def _say_hello_indept_streams(grpc_ch, service_client, gate):
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


_REQUESTS = {
    'request_without_case': _request_without_case,
    'say_hello': _say_hello,
    'say_hello_response_stream': _say_hello_response_stream,
    'say_hello_request_stream': _say_hello_request_stream,
    'say_hello_streams': _say_hello_streams,
    'say_hello_indept_streams': _say_hello_indept_streams,
}


async def unavailable_request(service_client, gate, case):
    response = await service_client.post(
        f'/hello?case={case}&timeout=small',
        data='Python',
        headers={'Content-type': 'text/plain'},
    )
    assert response.status == 500


def check_200_for(case):
    return _REQUESTS[case]


async def close_connection(gate, grpc_ch):
    gate.to_server_pass()
    gate.to_client_pass()
    await gate.sockets_close()
    await asyncio.wait_for(grpc_ch.channel_ready(), timeout=10)
