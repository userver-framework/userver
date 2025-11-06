HEAVY_OPERAIONS_IN_TRANSACTIONS = 'engine.heavy-operations-in-transactions'


async def test_trx_tracker_ignore_testpoint(service_client, monitor_client, testpoint):
    @testpoint('tp')
    def tp(data):
        pass

    response = await service_client.get('/handler')
    assert response.status == 200

    # Check that testpoint was called
    assert tp.times_called == 1

    # Check that only manual heavy operation in
    # transaction check was triggered
    metrics = await monitor_client.metrics(prefix=HEAVY_OPERAIONS_IN_TRANSACTIONS)
    assert metrics.value_at(path=HEAVY_OPERAIONS_IN_TRANSACTIONS) == 1
