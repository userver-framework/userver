import time

import pytest

from utils import consume


@pytest.mark.parametrize('timestamp_delta', [1000, 3000])
async def test_seek_timestamp(service_client, testpoint, kafka_producer, timestamp_delta):
    @testpoint('tp_kafka-consumer')
    def received_messages(_data):
        pass

    topic = f'test-topic-seek-{timestamp_delta}'
    await service_client.enable_testpoints()
    timestamp_ms = int(time.time() * 1000) - 10000
    for index in range(3):
        await kafka_producer.producer.send_and_wait(
            topic,
            key=str(index).encode(),
            value=str(index).encode(),
            partition=0,
            timestamp_ms=timestamp_ms + index * 1000,
        )

    response = await service_client.post(
        f'/consume/{topic}',
        json={'seek_timestamp_ms': timestamp_ms + timestamp_delta},
    )
    assert response.status_code == 200
    messages = response.json()['messages']
    # A marker also proves that a seek past the last record allows new records through.
    await kafka_producer.send(topic, 'marker', 'marker', partition=0)

    while not any(message['key'] == 'marker' for message in messages):
        await received_messages.wait_call()
        response = await consume(service_client, topic)
        messages.extend(response['messages'])

    expected_keys = ['1', '2', 'marker'] if timestamp_delta == 1000 else ['marker']
    assert [message['key'] for message in messages] == expected_keys
