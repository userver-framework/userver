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


async def send_all_data(sock):
    for data in DATA:
        await sock.sendall(data)


async def recv_all_data(sock):
    answer = b''
    while len(answer) < DATA_LENGTH:
        answer += await sock.recv(DATA_LENGTH - len(answer))

    assert answer == b''.join(DATA)


async def test_basic(service_client, asyncio_socket, monitor_client, tcp_service_port):
    await service_client.reset_metrics()

    sock = asyncio_socket.tcp()
    await sock.connect(('localhost', tcp_service_port))

    send_task = asyncio.create_task(send_all_data(sock))
    await recv_all_data(sock)
    await send_task
    metrics = await monitor_client.metrics(prefix='tcp-echo.')
    assert metrics.value_at('tcp-echo.sockets.opened') == 1
    assert metrics.value_at('tcp-echo.sockets.closed') == 0
    assert metrics.value_at('tcp-echo.bytes.read') == DATA_LENGTH
    # /// [Functional test]


@pytest.fixture(name='gate', scope='function')
async def _gate(tcp_service_port):
    gate_config = chaos.GateRoute(
        name='tcp proxy',
        host_to_server='localhost',
        port_to_server=tcp_service_port,
    )
    async with chaos.TcpGate(gate_config) as proxy:
        yield proxy


async def test_delay_recv(service_client, asyncio_socket, monitor_client, gate):
    await service_client.reset_metrics()
    timeout = 2.0

    # respond with delay in TIMEOUT seconds
    await gate.to_client_delay(timeout)

    sock = asyncio_socket.tcp()
    await sock.connect(gate.get_sockname_for_clients())
    sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

    recv_task = asyncio.create_task(recv_all_data(sock))
    await send_all_data(sock)

    done, _ = await asyncio.wait(
        [recv_task],
        timeout=timeout / 2,
        return_when=asyncio.FIRST_COMPLETED,
    )
    assert not done

    await gate.to_client_pass()

    await recv_task
    metrics = await monitor_client.metrics(prefix='tcp-echo.')
    assert metrics.value_at('tcp-echo.sockets.opened') == 1
    assert metrics.value_at('tcp-echo.sockets.closed') == 0
    assert metrics.value_at('tcp-echo.bytes.read') == DATA_LENGTH


async def test_data_combine(asyncio_socket, service_client, monitor_client, gate):
    await service_client.reset_metrics()
    await gate.to_client_concat_packets(DATA_LENGTH)

    sock = asyncio_socket.socket()
    await sock.connect(gate.get_sockname_for_clients())
    sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

    send_task = asyncio.create_task(send_all_data(sock))
    await recv_all_data(sock)
    await send_task

    await gate.to_client_pass()

    metrics = await monitor_client.metrics(prefix='tcp-echo.')
    assert metrics.value_at('tcp-echo.sockets.opened') == 1
    assert metrics.value_at('tcp-echo.sockets.closed') == 0
    assert metrics.value_at('tcp-echo.bytes.read') == DATA_LENGTH


async def test_down_pending_recv(service_client, asyncio_socket, gate):
    drop_queue = await gate.to_client_drop()

    sock = asyncio_socket.tcp()
    await sock.connect(gate.get_sockname_for_clients())
    sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

    async def _recv_no_data():
        data = await sock.recv(2)
        assert data == b''

    await send_all_data(sock)
    await drop_queue.wait_call()
    await gate.sockets_close()

    assert gate.connections_count() == 0
    await asyncio.wait_for(_recv_no_data(), timeout=10.0)

    await gate.to_client_pass()

    sock2 = asyncio_socket.tcp()
    await sock2.connect(gate.get_sockname_for_clients())
    await sock2.sendall(b'hi')
    hello = await sock2.recv(2)
    assert hello == b'hi'
    assert gate.connections_count() == 1


async def test_multiple_socks(
    asyncio_socket,
    service_client,
    monitor_client,
    tcp_service_port,
):
    await service_client.reset_metrics()
    sockets_count = 250

    tasks = []
    for _ in range(sockets_count):
        sock = asyncio_socket.tcp()
        await sock.connect(('localhost', tcp_service_port))
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        tasks.append(asyncio.create_task(send_all_data(sock)))
        tasks.append(asyncio.create_task(recv_all_data(sock)))
    await asyncio.gather(*tasks)

    metrics = await monitor_client.metrics(prefix='tcp-echo.')
    assert metrics.value_at('tcp-echo.sockets.opened') == sockets_count
    assert metrics.value_at('tcp-echo.bytes.read') == DATA_LENGTH * sockets_count


async def test_multiple_send_only(
    asyncio_socket,
    service_client,
    monitor_client,
    tcp_service_port,
):
    await service_client.reset_metrics()
    sockets_count = 25

    tasks = []
    for _ in range(sockets_count):
        sock = asyncio_socket.tcp()
        await sock.connect(('localhost', tcp_service_port))
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        tasks.append(asyncio.create_task(send_all_data(sock)))
    await asyncio.gather(*tasks)


async def test_metrics_smoke(monitor_client):
    metrics = await monitor_client.metrics()
    assert len(metrics) > 1
