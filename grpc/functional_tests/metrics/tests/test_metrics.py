import typing

import pytest
import samples.greeter_pb2 as greeter_protos  # noqa: E402, E501


def _normalize_metrics(metrics: str) -> typing.Set[str]:
    result = metrics.splitlines()

    result = _drop_non_grpc_metrics(result)
    result = _hide_metrics_values(result)

    return set(result)


def _drop_non_grpc_metrics(metrics: typing.List[str]) -> typing.List[str]:
    result = []
    for line in metrics:
        if line.startswith(('grpc.server', 'grpc.client')):
            result.append(line)

    return result


def _hide_metrics_values(metrics: typing.List[str]) -> typing.List[str]:
    return [line.rsplit('\t', 1)[0] for line in metrics]


@pytest.fixture(name='force_metrics_to_appear')
async def _force_metrics_to_appear(grpc_client, mock_grpc_greeter):
    @mock_grpc_greeter('SayHello')
    async def _mock_say_hello(mock_request, _mock_context):
        return greeter_protos.GreetingResponse(
            greeting=f'Hello, {mock_request.name} from mockserver!',
        )

    request = greeter_protos.GreetingRequest(name='Python')
    response = await grpc_client.SayHello(request)
    assert response.greeting == 'FWD: Hello, Python from mockserver!'

    assert _mock_say_hello.times_called == 1


async def test_metrics_smoke(monitor_client, force_metrics_to_appear):
    metrics = await monitor_client.metrics()
    assert len(metrics) > 1

    cache_hits = await monitor_client.single_metric(
        'grpc.server.by-destination.rps',
    )
    assert cache_hits.value > 0


async def test_metrics(monitor_client, load, force_metrics_to_appear):
    reference = _normalize_metrics(load('metrics_values.txt'))
    assert reference
    all_metrics = _normalize_metrics(
        await monitor_client.metrics_raw(output_format='pretty'),
    )
    assert all_metrics == reference
    assert all_metrics == reference, (
        '\n===== Service metrics start =====\n'
        f'{all_metrics}\n'
        '===== Service metrics end =====\n'
    )
