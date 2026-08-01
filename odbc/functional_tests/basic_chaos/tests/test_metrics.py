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
