async def test_grpc_compression_algorithm_metric(service_client, monitor_client):
    response = await service_client.post(
        'ydb/upsert-row',
        json={
            'id': 'id-compression',
            'name': 'name-compression',
            'service': 'srv',
            'channel': 123,
        },
    )
    assert response.status_code == 200
    assert response.json() == {}

    metrics = await monitor_client.metrics(prefix='ydb')
    assert (
        metrics.value_at(
            path='ydb.grpc-compression-algorithm',
            labels={'ydb_database': 'sampledb', 'algorithm': 'gzip'},
            default=0,
        )
        == 1
    )
