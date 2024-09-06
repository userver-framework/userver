import asyncio
import socket
import uuid

import pytest

DEFAULT_PATH = '/http2server'
DEFAULT_DATA = {'hello': 'world'}


async def test_http2_ping(http2_client):
    r = await http2_client.get('/ping')
    assert 200 == r.status_code
    assert '' == r.text


async def test_big_body(http2_client):
    s = 'x' * 2 ** 22  # request - 4Mib. limit - 2Mib
    r = await http2_client.get(
        DEFAULT_PATH, params={'type': 'echo-body'}, data=s,
    )
    assert 413 == r.status_code
    assert 'too large request' == r.text

    s = 'x' * 2 ** 20  # request - 1Mib. limit - 2Mib
    r = await http2_client.get(
        DEFAULT_PATH, params={'type': 'echo-body'}, data=s,
    )
    assert 200 == r.status_code
    assert s == r.text


async def test_json_body(http2_client):
    data = {'x': 'X', 'y': 'Y', 'd': 0.123, 'b': True, 'arr': [1, 2, 3, 4]}
    r = await http2_client.get(
        DEFAULT_PATH, params={'type': 'json'}, json=data,
    )
    assert 200 == r.status_code
    assert data == r.json()


async def test_headers(http2_client):
    hval = 'val'
    r = await http2_client.post(
        DEFAULT_PATH,
        params={'type': 'echo-header'},
        headers={'echo-header': hval, 'test': 'test'},
        json=DEFAULT_DATA,
    )
    assert 200 == r.status_code
    assert hval == r.text


async def _get_metric(monitor_client, metric_name):
    metric = await monitor_client.single_metric(
        f'server.requests.http2.{metric_name}',
    )
    return metric.value


async def _request(client, req_per_client, count=1):
    for _ in range(req_per_client):
        data = str(uuid.uuid4())
        data *= count
        r = await client.put(
            DEFAULT_PATH, params={'type': 'echo-body'}, data=data,
        )
        assert 200 == r.status_code
        assert data == r.text


async def test_concurrent_requests(
        http2_client, service_client, monitor_client,
):
    current_streams = await _get_metric(monitor_client, 'streams-count')
    clients_count = 10
    req_per_client = 100
    tasks = [
        _request(http2_client, req_per_client) for _ in range(clients_count)
    ]
    await asyncio.gather(*tasks)

    await service_client.update_server_state()

    metrics = await monitor_client.metrics(prefix='server.requests.http2')
    assert len(metrics) == 5
    total_requests = clients_count * req_per_client + current_streams
    assert total_requests == await _get_metric(monitor_client, 'streams-count')
    assert total_requests == await _get_metric(monitor_client, 'streams-close')
    assert 0 == await _get_metric(monitor_client, 'reset-streams')
    assert 0 == await _get_metric(monitor_client, 'goaway-streams')
    assert 0 == await _get_metric(monitor_client, 'streams-parse_error')


async def test_concurrent_requests_with_big_body(
        http2_client, service_client, monitor_client,
):
    current_streams = await _get_metric(monitor_client, 'streams-count')
    clients_count = 5
    req_per_client = 10
    count = int((2 ** 20) / 128)  # 1Mib / size(uuid)
    tasks = [
        _request(http2_client, req_per_client, count)
        for _ in range(clients_count)
    ]
    await asyncio.gather(*tasks)

    await service_client.update_server_state()

    metrics = await monitor_client.metrics(prefix='server.requests.http2')
    assert len(metrics) == 5
    total_requests = clients_count * req_per_client + current_streams
    assert total_requests == await _get_metric(monitor_client, 'streams-count')
    assert total_requests == await _get_metric(monitor_client, 'streams-close')
    assert 0 == await _get_metric(monitor_client, 'reset-streams')
    assert 0 == await _get_metric(monitor_client, 'goaway-streams')
    assert 0 == await _get_metric(monitor_client, 'streams-parse_error')


async def test_http1_ping(service_client):
    r = await service_client.get('/ping')
    assert r.status == 200


async def test_http1_broken_bytes(service_client, loop, service_port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    await loop.sock_connect(sock, ('localhost', service_port))
    await loop.sock_sendall(sock, 'GET / HTTP/1.1'.encode('utf-8'))
    sock.settimeout(1)
    with pytest.raises(TimeoutError):
        sock.recv(1024)
    await loop.sock_sendall(sock, 'garbage'.encode('utf-8'))
    r = await loop.sock_recv(sock, 1024)
    assert 'HTTP/1.1 400 Bad Request' in r.decode('utf-8')
    sock.close()
