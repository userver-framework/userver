import asyncio


_STATEMENT_QUERY_LABEL = 'odbc_query=odbc-functional-statement-metrics'


async def _statement_metric_lines(monitor_client):
    metrics = await monitor_client.metrics_raw(output_format='pretty')
    return [
        line
        for line in metrics.splitlines()
        if line.startswith('odbc.statement_')
        and _STATEMENT_QUERY_LABEL in line
    ]


async def _wait_for_statement_metrics(monitor_client):
    for _ in range(50):
        lines = await _statement_metric_lines(monitor_client)
        if lines:
            return lines
        await asyncio.sleep(0.02)
    return []


async def test_odbc_metrics_smoke(service_client, monitor_client):
    response = await service_client.post('/chaos?key=metrics&value=value')
    assert response.status == 201

    response = await service_client.get('/chaos/trx?key=metrics')
    assert response.status == 200

    metrics = await monitor_client.metrics_raw(output_format='pretty')
    odbc_metrics = [line for line in metrics.splitlines() if line.startswith('odbc.')]

    assert odbc_metrics
    assert any('component=key-value-db' in line for line in odbc_metrics)
    assert any('odbc_pool=0' in line for line in odbc_metrics)
    assert any(line.startswith('odbc.queries.executed') for line in odbc_metrics)
    assert any(line.startswith('odbc.transactions.committed') for line in odbc_metrics)


async def test_odbc_metrics_portability(service_client):
    warnings = await service_client.metrics_portability()
    assert not warnings


async def test_statement_metrics_generated_config_precedence_and_reset(
    service_client,
    monitor_client,
    dynamic_config,
):
    # Empty dynamic config falls back to static max_statement_metrics.
    response = await service_client.get('/statement-metrics')
    assert response.status == 200
    assert await _wait_for_statement_metrics(monitor_client)

    # __default__ overrides the static fallback and explicit zero clears it.
    dynamic_config.set(
        USERVER_ODBC_STATEMENT_METRICS_SETTINGS={
            '__default__': {'max_statement_metrics': 0},
        },
    )
    response = await service_client.get('/statement-metrics')
    assert response.status == 200
    assert not await _statement_metric_lines(monitor_client)

    # Exact component name wins over __default__.
    dynamic_config.set(
        USERVER_ODBC_STATEMENT_METRICS_SETTINGS={
            '__default__': {'max_statement_metrics': 0},
            'key-value-db': {'max_statement_metrics': 2},
        },
    )
    response = await service_client.get('/statement-metrics')
    assert response.status == 200
    assert await _wait_for_statement_metrics(monitor_client)

    dynamic_config.set(
        USERVER_ODBC_STATEMENT_METRICS_SETTINGS={
            '__default__': {'max_statement_metrics': 2},
            'key-value-db': {'max_statement_metrics': 0},
        },
    )
    response = await service_client.get('/statement-metrics')
    assert response.status == 200
    assert not await _statement_metric_lines(monitor_client)

    # Removing dynamic entries restores the static fallback.
    dynamic_config.set(USERVER_ODBC_STATEMENT_METRICS_SETTINGS={})
    response = await service_client.get('/statement-metrics')
    assert response.status == 200
    lines = await _wait_for_statement_metrics(monitor_client)
    assert all('component=key-value-db' in line for line in lines)
    assert all('odbc_pool=0' in line for line in lines)
    assert all('SELECT 1' not in line for line in lines)
    assert any(line.startswith('odbc.statement_timings') for line in lines)
    assert any(line.startswith('odbc.statement_executed') for line in lines)
    assert any(line.startswith('odbc.statement_errors') for line in lines)
