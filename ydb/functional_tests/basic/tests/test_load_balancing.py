async def test_grpc_load_balancing_policy_metric(service_client, monitor_client):
    response = await service_client.post(
        'ydb/select-rows',
        json={
            'service': 'srv',
            'channels': [1, 2, 3],
            'created': '2019-10-30T11:20:00+00:00',
        },
    )
    assert response.status_code == 200

    metrics = await monitor_client.metrics(prefix='ydb')
    assert (
        metrics.value_at(
            path='ydb.grpc-load-balancing-policy',
            labels={'ydb_database': 'sampledb', 'policy': 'round_robin'},
            default=0,
        )
        == 1
    )
