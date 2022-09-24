# /// [Functional test]
import asyncio
import socket

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


async def test_multiple_socks(service_client, loop, monitor_client):
    await service_client.reset_metrics()
    sockets_count = 250

    tasks = []
    for _ in range(sockets_count):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        await loop.sock_connect(s, ('localhost', 8181))
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
        tasks.append(asyncio.create_task(send_all_data(s, loop)))
    await asyncio.gather(*tasks)
