async def test_basic(service_client, monitor_client):
    response = await service_client.get('/metrics')
    assert response.status_code == 200

    metric = await monitor_client.single_metric('sample-metrics.foo')
    assert metric.value > 0


# /// [metrics reset]
async def test_reset(service_client, monitor_client):
    # Reset service metrics
    await service_client.reset_metrics()
    # Retrieve metrics
    metric = await monitor_client.single_metric('sample-metrics.foo')
    assert metric.value == 0
    assert not metric.labels
    # /// [metrics reset]

    response = await service_client.get('/metrics')
    assert response.status_code == 200

    metric = await monitor_client.single_metric('sample-metrics.foo')
    assert metric.value == 1

    await service_client.reset_metrics()
    metric = await monitor_client.single_metric('sample-metrics.foo')
    assert metric.value == 0


# /// [metrics labels]
async def test_engine_metrics(service_client, monitor_client):
    metric = await monitor_client.single_metric(
        'engine.task-processors.tasks.finished',
        labels={'task_processor': 'main-task-processor'},
    )
    assert metric.value > 0
    assert metric.labels == {'task_processor': 'main-task-processor'}

    metrics_dict = await monitor_client.metrics(
        prefix='http.', labels={'http_path': '/ping'},
    )
    assert metrics_dict
    assert 'http.handler.cancelled-by-deadline' in metrics_dict
    # /// [metrics labels]
    assert 'http.handler.in-flight' in metrics_dict
    assert 'http.handler.too-many-requests-in-flight' in metrics_dict
    assert 'http.handler.rate-limit-reached' in metrics_dict
    assert 'http.handler.deadline-received' in metrics_dict
    assert 'http.handler.timings' in metrics_dict
    assert 'http.handler.reply-codes' in metrics_dict

    for key, value in metrics_dict.items():
        assert value, f'Error at {key}'
        for metric in value:
            assert 'http_path' in metric.labels, f'Error at {key}'
            assert metric.labels['http_path'] == '/ping', f'Error at {key}'

            assert 'http_handler' in metric.labels, f'Error at {key}'
            assert metric.labels['http_handler'] == 'handler-ping', (
                f'Error at {key}',
            )

    metrics_dict = await monitor_client.metrics(
        prefix='http.handler.too-many-requests-in-flight',
        labels={'http_path': '/ping'},
    )
    assert 'http.handler.too-many-requests-in-flight' in metrics_dict
    assert len(metrics_dict) == 1
    assert len(metrics_dict['http.handler.too-many-requests-in-flight']) == 1

    metrics_dict = await monitor_client.metrics(
        path='http.handler.too-many-requests-in-flight',
        labels={'http_path': '/ping'},
    )
    assert 'http.handler.too-many-requests-in-flight' in metrics_dict
    assert len(metrics_dict) == 1
    assert len(metrics_dict['http.handler.too-many-requests-in-flight']) == 1

    metric = await monitor_client.single_metric(
        'http.handler.timings',
        labels={'percentile': 'p95', 'http_path': '/ping'},
    )
    assert metric

    metric = await monitor_client.single_metric(
        'http.handler.timings',
        labels={'http_path': '/ping', 'percentile': 'p99_9'},
    )
    assert metric


# /// [metrics portability]
async def test_metrics_portability(service_client):
    warnings = await service_client.metrics_portability()
    assert not warnings, warnings
    # /// [metrics portability]


# /// [metrics partial portability]
async def test_partial_metrics_portability(service_client):
    warnings = await service_client.metrics_portability()
    nan_metric_values = warnings.get('nan')
    assert not nan_metric_values, nan_metric_values
    # /// [metrics partial portability]
