import re

from utils import consume
from utils import produce

TOPIC1 = 'test-topic-consume-produced'


def _normalize_metrics(metrics: str) -> str:
    result = []
    for line in metrics.splitlines():
        line = line.strip()
        if line.startswith('#') or not line:
            continue

        left, _ = line.rsplit('\t', 1)
        result.append(
            re.sub('localhost:\\d+', 'localhost:00000', left + '\t' + '0'),
        )
    result.sort()
    return '\n'.join(result)


async def test_metrics_consumer_smoke(monitor_client):
    metrics = await monitor_client.metrics(prefix='kafka_consumer.')
    assert len(metrics) >= 1


async def test_metrics_producer_smoke(monitor_client):
    metrics = await monitor_client.metrics(prefix='kafka_producer.')
    assert len(metrics) >= 1


async def test_one_producer_sync_one_consumer_one_topic(
    service_client,
    monitor_client,
    testpoint,
    load,
):
    @testpoint('tp_kafka-consumer')
    def received_messages_func(_data):
        pass

    await service_client.enable_testpoints()
    await service_client.reset_metrics()

    await produce(service_client, TOPIC1, 'test-key', 'test-message')

    await received_messages_func.wait_call()

    ethalon = _normalize_metrics(load('metrics_values_producer.txt'))
    assert ethalon
    all_metrics = _normalize_metrics(
        await monitor_client.metrics_raw(
            output_format='pretty',
            prefix='kafka_producer.',
        ),
    )
    assert all_metrics == ethalon

    await consume(service_client, TOPIC1)

    ethalon = _normalize_metrics(load('metrics_values_consumer.txt'))
    assert ethalon
    all_metrics = _normalize_metrics(
        await monitor_client.metrics_raw(
            output_format='pretty',
            prefix='kafka_consumer.',
        ),
    )
    assert all_metrics == ethalon
