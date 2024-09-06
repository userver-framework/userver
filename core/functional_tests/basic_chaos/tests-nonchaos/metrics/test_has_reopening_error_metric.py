import os


async def test_metrics_has_reopening_error(
        monitor_client, service_config_yaml,
):
    assert await monitor_client.fired_alerts() == []

    metrics = await monitor_client.metrics()
    assert len(metrics) > 1
    has_reopening_error = await monitor_client.single_metric(
        'logger.has_reopening_error',
    )
    assert has_reopening_error.value == 0

    log_file_name = service_config_yaml['components_manager']['components'][
        'logging'
    ]['loggers']['default']['file_path']

    os.chmod(log_file_name, 0o111)
    response = await monitor_client.post('/service/on-log-rotate/')
    assert response.status == 500

    has_reopening_error = await monitor_client.single_metric(
        'logger.has_reopening_error',
    )
    assert has_reopening_error.value == 1

    assert await monitor_client.fired_alerts() == [
        {
            'id': 'log_reopening_error',
            'message': (
                'loggers [default]'
                + ' failed to reopen the log file: logs are getting lost now'
            ),
        },
    ]

    os.chmod(log_file_name, 0o664)
    response = await monitor_client.post('/service/on-log-rotate/')
    assert response.status == 200

    has_reopening_error = await monitor_client.single_metric(
        'logger.has_reopening_error',
    )
    assert has_reopening_error.value == 0

    assert await monitor_client.fired_alerts() == []
