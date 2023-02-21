# /// [Functional test]
import asyncio
import socket

import pytest
from pytest_userver import chaos

DATA = (
    b'Once upon a midnight dreary, while I pondered, weak and weary, ',
    b'Over many a quaint and curious volume of forgotten lore - ',
    b'While I nodded, nearly napping, suddenly there came a tapping, ',
    b'As of some one gently rapping, rapping at my chamber door - ',
    b'"Tis some visitor," I muttered, "tapping at my chamber door - ',
    b'Only this and nothing more."',
)
DATA_LENGTH = sum(len(x) for x in DATA)


# Another way to say that monitor handlers listen for the main service port
@pytest.fixture(scope='session')
def monitor_port(service_port) -> int:
    return service_port


async def send_all_data(s, loop):
    for data in DATA:
        await loop.sock_sendall(s, data)


async def recv_all_data(s, loop):
    answer = b''
    while len(answer) < DATA_LENGTH:
        answer += await loop.sock_recv(s, DATA_LENGTH - len(answer))

    assert answer == b''.join(DATA)


async def test_basic(service_client, loop, monitor_client, tcp_service_port):
    await service_client.reset_metrics()

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    await loop.sock_connect(sock, ('localhost', tcp_service_port))

    send_task = asyncio.create_task(send_all_data(sock, loop))
    await recv_all_data(sock, loop)
    await send_task
    metrics = await monitor_client.metrics(prefix='tcp-echo.')
    assert metrics.value_at('tcp-echo.sockets.opened') == 1
    assert metrics.value_at('tcp-echo.sockets.closed') == 0
    assert metrics.value_at('tcp-echo.bytes.read') == DATA_LENGTH
    # /// [Functional test]


@pytest.fixture(name='gate', scope='function')
async def _gate(loop, tcp_service_port):
    gate_config = chaos.GateRoute(
        name='tcp proxy',
        host_to_server='localhost',
        port_to_server=tcp_service_port,
    )
    async with chaos.TcpGate(gate_config, loop) as proxy:
        yield proxy


async def test_delay_recv(service_client, loop, monitor_client, gate):
    await service_client.reset_metrics()
    TIMEOUT = 10.0

    # respond with delay in TIMEOUT seconds
    gate.to_client_delay(TIMEOUT)

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    await loop.sock_connect(sock, gate.get_sockname_for_clients())
    sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

    recv_task = asyncio.create_task(recv_all_data(sock, loop))
    await send_all_data(sock, loop)

    done, _ = await asyncio.wait(
        [recv_task], timeout=TIMEOUT / 2, return_when=asyncio.FIRST_COMPLETED,
    )
    assert not done

    gate.to_client_pass()

    await recv_task
    metrics = await monitor_client.metrics(prefix='tcp-echo.')
    assert metrics.value_at('tcp-echo.sockets.opened') == 1
    assert metrics.value_at('tcp-echo.sockets.closed') == 0
    assert metrics.value_at('tcp-echo.bytes.read') == DATA_LENGTH


async def test_data_combine(service_client, loop, monitor_client, gate):
    await service_client.reset_metrics()
    gate.to_client_concat_packets(DATA_LENGTH)

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    await loop.sock_connect(sock, gate.get_sockname_for_clients())
    sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

    send_task = asyncio.create_task(send_all_data(sock, loop))
    await recv_all_data(sock, loop)
    await send_task

    gate.to_client_pass()

    metrics = await monitor_client.metrics(prefix='tcp-echo.')
    assert metrics.value_at('tcp-echo.sockets.opened') == 1
    assert metrics.value_at('tcp-echo.sockets.closed') == 0
    assert metrics.value_at('tcp-echo.bytes.read') == DATA_LENGTH


async def test_down_pending_recv(service_client, loop, gate):
    gate.to_client_noop()

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    await loop.sock_connect(sock, gate.get_sockname_for_clients())
    sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

    async def _recv_no_data(s, loop):
        answer = b''
        try:
            while True:
                answer += await loop.sock_recv(sock, 2)
                assert False
        except Exception:  # pylint: disable=broad-except
            pass

        assert answer == b''

    recv_task = asyncio.create_task(_recv_no_data(sock, loop))

    await send_all_data(sock, loop)

    await asyncio.wait(
        [recv_task], timeout=1, return_when=asyncio.FIRST_COMPLETED,
    )
    await gate.sockets_close()
    await recv_task
    assert gate.connections_count() == 0

    gate.to_client_pass()

    sock2 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock2.connect(gate.get_sockname_for_clients())
    await loop.sock_sendall(sock2, b'hi')
    hello = await loop.sock_recv(sock2, 2)
    assert hello == b'hi'
    assert gate.connections_count() == 1


async def test_multiple_socks(
        service_client, loop, monitor_client, tcp_service_port,
):
    await service_client.reset_metrics()
    sockets_count = 250

    tasks = []
    for _ in range(sockets_count):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        await loop.sock_connect(sock, ('localhost', tcp_service_port))
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        tasks.append(asyncio.create_task(send_all_data(sock, loop)))
        tasks.append(asyncio.create_task(recv_all_data(sock, loop)))
    await asyncio.gather(*tasks)

    metrics = await monitor_client.metrics(prefix='tcp-echo.')
    assert metrics.value_at('tcp-echo.sockets.opened') == sockets_count
    assert (
        metrics.value_at('tcp-echo.bytes.read') == DATA_LENGTH * sockets_count
    )


async def test_multiple_send_only(
        service_client, loop, monitor_client, tcp_service_port,
):
    await service_client.reset_metrics()
    sockets_count = 25

    tasks = []
    for _ in range(sockets_count):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        await loop.sock_connect(sock, ('localhost', tcp_service_port))
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        tasks.append(asyncio.create_task(send_all_data(sock, loop)))
    await asyncio.gather(*tasks)


async def test_metrics_smoke(monitor_client):
    metrics = await monitor_client.metrics()
    assert len(metrics) > 1
