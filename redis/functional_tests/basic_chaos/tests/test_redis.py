import asyncio
import uuid

import pytest
import pytest_userver.utils.sync as sync

PUBSUB_CHANNEL = 'chaos_pubsub_recovery'
PUBSUB_URL = '/chaos-pubsub'
PUBSUB_MESSAGE_WAIT_SECONDS = 1


async def _wait_for_pubsub_message(redis_store, service_client, message):
    async def publish_and_wait():
        unique_message = f'{message}:{uuid.uuid4()}'
        redis_store.publish(PUBSUB_CHANNEL, unique_message)

        async def check_received():
            response = await service_client.get(PUBSUB_URL)
            assert response.status == 200
            if unique_message not in response.json()['data']:
                raise sync.NotReady()

        try:
            await sync.wait_until(check_received, total_wait_seconds=PUBSUB_MESSAGE_WAIT_SECONDS)
        except TimeoutError:
            raise sync.NotReady() from None

    await sync.wait_until(publish_and_wait, total_wait_seconds=60)


async def _check_that_restores(client, gate):
    try:
        await gate.wait_for_connections(timeout=10.0)
    except asyncio.TimeoutError:
        assert False, 'Timeout while waiting for restore'

    assert gate.connections_count() >= 1

    async def is_ready():
        res = await client.delete('/chaos?key=foo')
        return res.status == 200

    await sync.wait(is_ready)


async def _check_crud(client):
    response = await client.post('/chaos?key=foo&value=bar')
    assert response.status == 201

    response = await client.get('/chaos?key=foo')
    assert response.status == 200
    assert response.text == 'bar'

    response = await client.post('/chaos?key=foo&value=bar2')
    assert response.status == 409

    response = await client.delete('/chaos?key=foo')
    assert response.status == 200


async def test_redis_happy(service_client, sentinel_gate, gate):
    await _check_crud(service_client)


@pytest.mark.skip(reason='Flaky test TAXICOMMON-6075')
async def test_smaller_parts(service_client, sentinel_gate, gate):
    await gate.to_server_smaller_parts(20)
    await gate.to_client_smaller_parts(20)
    await _check_crud(service_client)


@pytest.mark.skip(reason='Flaky test TAXICOMMON-6075')
async def test_redis_disable_reads(service_client, sentinel_gate, gate):
    await gate.to_server_noop()
    result = await service_client.delete('/chaos?key=foo')
    assert result.status == 500
    await gate.to_server_pass()
    await _check_that_restores(service_client, gate)


async def test_redis_disable_writes(service_client, sentinel_gate, gate):
    await gate.to_client_noop()
    result = await service_client.delete('/chaos?key=foo')
    assert result.status == 500
    await gate.to_client_pass()
    await _check_that_restores(service_client, gate)


async def test_redis_close_connections(service_client, sentinel_gate, gate):
    response = await service_client.get('/chaos?key=foo')
    assert response.status == 404

    await gate.sockets_close()
    await _check_that_restores(service_client, gate)


async def test_pubsub_recovers_after_network_outage(service_client, redis_store, sentinel_gate, pubsub_gate):
    response = await service_client.delete(PUBSUB_URL)
    assert response.status == 200
    await _wait_for_pubsub_message(redis_store, service_client, 'before_outage')

    response = await service_client.delete(PUBSUB_URL)
    assert response.status == 200

    await pubsub_gate.stop_accepting()
    try:
        await pubsub_gate.sockets_close()
        assert pubsub_gate.connections_count() == 0

        redis_store.publish(PUBSUB_CHANNEL, 'during_outage')
        response = await service_client.get(PUBSUB_URL)
        assert response.status == 200
        assert response.json()['data'] == []
    finally:
        pubsub_gate.start_accepting()

    await pubsub_gate.wait_for_connections(timeout=10.0)
    await _wait_for_pubsub_message(redis_store, service_client, 'after_outage')
