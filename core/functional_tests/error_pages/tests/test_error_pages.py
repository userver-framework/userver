import socket

import pytest


@pytest.fixture(scope='session')
def error_page(service_source_dir) -> str:
    return (service_source_dir / 'index.html').read_text()


async def test_handler_response_is_not_affected(service_client):
    response = await service_client.get('/hello')
    assert response.status == 200
    assert response.text == 'Hello world!\n'
    assert 'X-Powered-By' not in response.headers


async def test_unknown_path(service_client, error_page):
    response = await service_client.get('/no/such/path')
    assert response.status == 200
    assert response.text == error_page
    assert response.headers['Content-Type'] == 'text/html'
    assert response.headers['X-Powered-By'] == 'userver'


async def test_method_not_allowed(service_client, error_page):
    response = await service_client.post('/hello')
    assert response.status == 200
    assert response.text == error_page
    assert response.headers['X-Powered-By'] == 'userver'


async def test_status_is_kept_if_not_configured(service_client):
    # The URI is longer than the default 'max_url_size' of 8192 bytes.
    response = await service_client.get('/' + 'x' * 9000)
    assert response.status == 414
    assert response.text == 'the URI is too long'


async def test_head_request_has_no_body(service_client, error_page):
    response = await service_client.request('HEAD', '/no/such/path')
    assert response.status == 200
    assert response.content == b''
    assert response.headers['Content-Length'] == str(len(error_page))


# A malformed request is rejected by the parser, before the routing takes
# place; both layers must end up on the same error page.
async def test_malformed_request(service_client, service_port, error_page):
    with socket.create_connection(('localhost', service_port), timeout=10) as sock:
        sock.sendall(b'FOOBAR / HTTP/1.1\r\nHost: localhost\r\n\r\n')
        sock.shutdown(socket.SHUT_WR)
        response = b''
        while chunk := sock.recv(4096):
            response += chunk

    assert response.startswith(b'HTTP/1.1 200 OK\r\n'), response
    assert response.endswith(error_page.encode()), response
    assert b'\r\nX-Powered-By: userver\r\n' in response
