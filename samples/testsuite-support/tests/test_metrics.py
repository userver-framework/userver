import pytest
import pytest_userver.client
import pytest_userver.metrics


async def test_basic(service_client, monitor_client):
    response = await service_client.get('/metrics')
    assert response.status_code == 200
    assert 'application/json' in response.headers['Content-Type']

    metric = await monitor_client.single_metric('sample-metrics.foo')
    assert metric.value > 0


# /// [uservice_oneshot sample]
@pytest.mark.uservice_oneshot
async def test_initial_metrics(service_client, monitor_client):
    metric = await monitor_client.single_metric('sample-metrics.foo')
    assert metric.value == 0
    # /// [uservice_oneshot sample]


# /// [metrics reset]
async def test_reset(service_client: pytest_userver.client.Client, monitor_client: pytest_userver.client.ClientMonitor):
    # Reset service metrics via calling `ResetMetric` for all the metrics that have such function
    await service_client.reset_metrics()

    # Retrieve metrics
    metric: pytest_userver.metrics.Metric = await monitor_client.single_metric('sample-metrics.foo')
    assert metric.value == 0
    assert not metric.labels
    # /// [metrics reset]

    response = await service_client.get('/metrics')
    assert response.status_code == 200
    assert 'application/json' in response.headers['Content-Type']

    metric = await monitor_client.single_metric('sample-metrics.foo')
    assert metric.value == 1

    await service_client.reset_metrics()
    metric = await monitor_client.single_metric('sample-metrics.foo')
    assert metric.value == 0


# /// [metrics labels]
async def test_engine_metrics(service_client, monitor_client: pytest_userver.client.ClientMonitor):
    metric: pytest_userver.metrics.Metric = await monitor_client.single_metric(
        'engine.task-processors.tasks.finished.v2',
        labels={'task_processor': 'main-task-processor'},
    )
    assert metric.value > 0
    assert metric.labels == {'task_processor': 'main-task-processor'}

    metrics_dict: pytest_userver.metrics.MetricsSnapshot = await monitor_client.metrics(
        prefix='http.',
        labels={'http_path': '/ping'},
    )

    assert metrics_dict
    assert 'http.handler.cancelled-by-deadline' in metrics_dict

    assert (
        metrics_dict.value_at(
            'http.handler.in-flight',
            labels={
                'http_path': '/ping',
                'http_handler': 'handler-ping',
                'version': '2',
            },
        )
        == 0
    )
    # /// [metrics labels]


# /// [metrics single_metric]
async def test_engine_tasks_alive_metric(service_client, monitor_client: pytest_userver.client.ClientMonitor):
    metric: pytest_userver.metrics.Metric = await monitor_client.single_metric(
        'engine.task-processors.tasks.alive',
        labels={'task_processor': 'main-task-processor'},
    )
    assert metric.value > 0
    assert metric.labels == {'task_processor': 'main-task-processor'}
    # /// [metrics single_metric]


# /// [metrics single_metric_optional]
async def test_some_optional_metric(service_client, monitor_client: pytest_userver.client.ClientMonitor):
    metric: pytest_userver.metrics.Metric | None = await monitor_client.single_metric_optional(
        'some.metric.error',
        labels={'task_processor': 'main-task-processor'},
    )
    assert metric is None or metric.value == 0
    # /// [metrics single_metric_optional]


# /// [metrics diff]
async def test_diff_metrics(service_client, monitor_client: pytest_userver.client.ClientMonitor):
    async with monitor_client.metrics_diff(prefix='sample-metrics') as differ:
        # Do something that makes the service update its metrics
        response = await service_client.get('/metrics')
        assert response.status == 200

    # Checking diff of 'sample-metrics.foo' metric
    assert differ.value_at('foo') == 1
    # /// [metrics diff]


# /// [metrics metrics]
async def test_engine_logger_metrics(service_client, monitor_client: pytest_userver.client.ClientMonitor):
    metrics_dict: pytest_userver.metrics.MetricsSnapshot = await monitor_client.metrics(
        prefix='logger.',
        labels={'logger': 'default'},
    )

    assert metrics_dict
    assert 'logger.total' in metrics_dict
    assert metrics_dict.value_at('logger.total') > 0

    assert (
        metrics_dict.value_at(
            'logger.dropped',
            labels={
                'logger': 'default',
                'version': '2',
            },
        )
        == 0
    )
    # /// [metrics metrics]
