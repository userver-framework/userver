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


async def send_all_data(s, loop):
    for data in DATA:
        await loop.sock_sendall(s, data)


async def recv_all_data(s, loop):
    answer = b''
    while len(answer) < DATA_LENGTH:
        answer += await loop.sock_recv(s, DATA_LENGTH - len(answer))

    assert answer == b''.join(DATA)


async def test_basic(service_client, loop, monitor_client):
    await service_client.reset_metrics()

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    await loop.sock_connect(s, ('localhost', 8181))

    send_task = asyncio.create_task(send_all_data(s, loop))
    await recv_all_data(s, loop)
    await send_task
    metrics = await monitor_client.get_metrics()
    assert metrics['tcp-echo']['sockets']['opened'] == 1
    assert metrics['tcp-echo']['sockets']['closed'] == 0
    assert metrics['tcp-echo']['bytes']['read'] == DATA_LENGTH
    # /// [Functional test]


@pytest.fixture(name='gate', scope='function')
async def _gate(loop):
    gate_config = chaos.GateRoute(
        name='tcp proxy',
        host_left='localhost',
        port_left=9181,
        host_right='localhost',
        port_right=8181,
    )
    async with chaos.TcpGate(gate_config, loop) as proxy:
        yield proxy


async def test_delay_recv(service_client, loop, monitor_client, gate):
    await service_client.reset_metrics()
    TIMEOUT = 10.0

    # respond with delay in TIMEOUT seconds
    gate.to_left_delay(TIMEOUT)

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    await loop.sock_connect(s, ('localhost', 9181))
    s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

    recv_task = asyncio.create_task(recv_all_data(s, loop))
    await send_all_data(s, loop)

    done, _ = await asyncio.wait(
        [recv_task], timeout=TIMEOUT / 2, return_when=asyncio.FIRST_COMPLETED,
    )
    assert not done

    gate.to_left_pass()

    await recv_task
    metrics = await monitor_client.get_metrics()
    assert metrics['tcp-echo']['sockets']['opened'] == 1
    assert metrics['tcp-echo']['sockets']['closed'] == 0
    assert metrics['tcp-echo']['bytes']['read'] == DATA_LENGTH


async def test_down_pending_recv(service_client, loop, monitor_client, gate):
    await service_client.reset_metrics()

    gate.to_left_noop()

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    await loop.sock_connect(s, ('localhost', 9181))
    s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

    async def _recv_no_data(s, loop):
        answer = b''
        try:
            while True:
                answer += await loop.sock_recv(s, 2)
                assert False
        except Exception:  # pylint: disable=broad-except
            pass

        assert answer == b''

    recv_task = asyncio.create_task(_recv_no_data(s, loop))

    await send_all_data(s, loop)

    await gate.stop()

    await recv_task


async def test_multiple_socks(service_client, loop, monitor_client):
    await service_client.reset_metrics()
    sockets_count = 250

    tasks = []
    for _ in range(sockets_count):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        await loop.sock_connect(s, ('localhost', 8181))
        s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        tasks.append(asyncio.create_task(send_all_data(s, loop)))
        tasks.append(asyncio.create_task(recv_all_data(s, loop)))
    await asyncio.gather(*tasks)

    metrics = await monitor_client.get_metrics()
    assert metrics['tcp-echo']['sockets']['opened'] == sockets_count
    assert metrics['tcp-echo']['bytes']['read'] == DATA_LENGTH * sockets_count


async def test_multiple_send_only(service_client, loop, monitor_client):
    await service_client.reset_metrics()
    sockets_count = 25

    tasks = []
    for _ in range(sockets_count):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        await loop.sock_connect(s, ('localhost', 8181))
        s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        tasks.append(asyncio.create_task(send_all_data(s, loop)))
    await asyncio.gather(*tasks)
