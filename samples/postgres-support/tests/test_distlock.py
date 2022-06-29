async def test_distlock(service_client, testpoint):
    @testpoint('distlock-worker')
    def distlock_tp(data):
        pass

    await service_client.run_distlock_task('distlock-pg')
    assert distlock_tp.times_called == 1
